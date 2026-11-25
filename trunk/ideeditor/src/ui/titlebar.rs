use crate::{
    model::style_config::{EditorStyleConfig, UiSkin},
    ui::theme::draw_glass_bar_background,
};
use fltk::{
    app, draw,
    enums::{Align, ColorDepth, Cursor, Event, Font, FrameType},
    group::{Flex, FlexType},
    image::RgbImage,
    prelude::*,
    widget::Widget,
    window::Window,
};
use std::{cell::RefCell, collections::HashMap, rc::Rc};

pub const TITLEBAR_HEIGHT: i32 = 38;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
struct DotsCacheKey {
    phys_w: i32,
    phys_h: i32,
    panel_rgb: (u8, u8, u8),
    is_dark: bool,
    ui_skin: UiSkin,
    hovered_dot: Option<usize>,
    pressed_dot: Option<usize>,
}

#[derive(Clone)]
struct HeaderThemeState {
    panel_rgb: (u8, u8, u8),
    text_rgb: (u8, u8, u8),
    is_dark: bool,
    ui_skin: UiSkin,
    title: String,
    hovered_dot: Option<usize>,
    pressed_dot: Option<usize>,
    watch_active: bool,
    dots_cache: HashMap<DotsCacheKey, RgbImage>,
}

#[derive(Clone, Copy, Default)]
struct DragSession {
    active: bool,
    start_root: (i32, i32),
    offset: (i32, i32),
}

/// Кастомная безрамочная шапка окна в стиле macOS / Linux с 3 чёткими цветными точками в углу.
#[derive(Clone)]
pub struct CustomTitleBar {
    pub bar: Flex,
    dots_widget: Widget,
    drag_widget: Widget,
    state: Rc<RefCell<HeaderThemeState>>,
}

impl CustomTitleBar {
    /// Создаёт безрамочную шапку с тремя кнопками-точками (Закрыть / Свернуть / Развернуть)
    /// и областью перетаскивания окна мышью.
    pub fn new<F, C>(
        win: &Window,
        initial_title: &str,
        cfg: &EditorStyleConfig,
        allow_maximize: bool,
        mut on_close: C,
        build_extra_left: F,
    ) -> Self
    where
        F: FnOnce(&mut Flex),
        C: FnMut() + 'static,
    {
        let chrome = cfg.chrome_palette();
        let state = Rc::new(RefCell::new(HeaderThemeState {
            panel_rgb: chrome.panel_bg,
            text_rgb: chrome.text_fg,
            is_dark: chrome.is_dark,
            ui_skin: cfg.ui_skin,
            title: initial_title.to_string(),
            hovered_dot: None,
            pressed_dot: None,
            watch_active: false,
            dots_cache: HashMap::new(),
        }));

        let mut bar = Flex::default().with_type(FlexType::Row);
        bar.set_frame(FrameType::NoBox);
        bar.set_margins(4, 0, 8, 0);
        bar.set_pad(4);

        // 1. Три круглые цветные точки управления окном (HiDPI 16x MSAA + мягкое затемнение/вдавливание при наведении)
        let mut dots_widget = Widget::default();
        bar.fixed(&dots_widget, 74);

        // 2. Опциональные элементы слева (компактное меню Файл / Вид в главном окне)
        build_extra_left(&mut bar);

        // 3. Растягиваемая центральная зона заголовка (перетаскивание окна + двойной клик)
        let mut drag_widget = Widget::default();

        bar.end();

        // ВАЖНО: при перерисовке фона шапки принудительно отрисовываем КАЖДЫЙ дочерний виджет (`draw_child`),
        // чтобы при частичном обновлении (`Damage::Child`) соседние элементы никогда не затирались фоном!
        {
            let st = Rc::clone(&state);
            bar.draw(move |b| {
                let (panel_rgb, is_dark, ui_skin) = {
                    let s = st.borrow();
                    (s.panel_rgb, s.is_dark, s.ui_skin)
                };
                draw_glass_bar_background(
                    b.x(),
                    b.y(),
                    b.w(),
                    b.h(),
                    panel_rgb,
                    is_dark,
                    ui_skin,
                    true,
                );
                for idx in 0..b.children() {
                    if let Some(mut child) = b.child(idx) {
                        b.draw_child(&mut child);
                    }
                }
            });
        }

        // Отрисовка 3 цветных точек в физическом HiDPI-разрешении монитора с кэшированием состояний наведения
        {
            let st = Rc::clone(&state);
            let win_dpi = win.clone();
            dots_widget.draw(move |w| {
                let dpi = win_dpi.pixels_per_unit().clamp(1.0, 4.0);
                let phys_w = ((w.w() as f32) * dpi).round().max(4.0) as i32;
                let phys_h = ((w.h() as f32) * dpi).round().max(4.0) as i32;

                let mut s = st.borrow_mut();
                if s.dots_cache.len() > 24 {
                    s.dots_cache.clear();
                }

                let key = DotsCacheKey {
                    phys_w,
                    phys_h,
                    panel_rgb: s.panel_rgb,
                    is_dark: s.is_dark,
                    ui_skin: s.ui_skin,
                    hovered_dot: s.hovered_dot,
                    pressed_dot: s.pressed_dot,
                };

                if !s.dots_cache.contains_key(&key)
                    && let Some(mut img) = build_hidpi_traffic_lights(key, dpi, allow_maximize)
                {
                    img.scale(w.w(), w.h(), false, true);
                    s.dots_cache.insert(key, img);
                }

                if let Some(img) = s.dots_cache.get_mut(&key) {
                    img.draw(w.x(), w.y(), w.w(), w.h());
                }
            });
        }

        // Интерактивное наведение (затемнение + лёгкое вдавливание) и клик по 3 точкам
        {
            let st = Rc::clone(&state);
            let mut win_ctrl = win.clone();
            dots_widget.handle(move |w, ev| {
                let active_hit = |mx: i32, my: i32| {
                    hit_test_dot(w.x(), w.y(), w.h(), mx, my)
                        .filter(|&idx| idx < 2 || allow_maximize)
                };

                // Запускает лёгкий таймер проверки реальной позиции курсора ОС (`app::get_mouse`),
                // чтобы эффект наведения гарантированно снимался при уходе мыши за край безрамочного окна,
                // при сворачивании, разворачивании или открытии диалогов.
                let ensure_hover_watchdog =
                    |st_ref: &Rc<RefCell<HeaderThemeState>>, wid: &Widget, win_ref: &Window| {
                        let should_start = {
                            let mut s = st_ref.borrow_mut();
                            if s.hovered_dot.is_some() && !s.watch_active {
                                s.watch_active = true;
                                true
                            } else {
                                false
                            }
                        };
                        if should_start {
                            let st_w = Rc::clone(st_ref);
                            let mut wid_w = wid.clone();
                            let win_w = win_ref.clone();
                            app::add_timeout3(0.05, move |handle| {
                                let mut s = st_w.borrow_mut();
                                if s.pressed_dot.is_some() {
                                    app::repeat_timeout3(0.05, handle);
                                    return;
                                }
                                if !win_w.shown() {
                                    s.hovered_dot = None;
                                    s.watch_active = false;
                                    drop(s);
                                    wid_w.redraw();
                                    return;
                                }
                                let (mx_root, my_root) = app::get_mouse();
                                let mx_local = mx_root - win_w.x();
                                let my_local = my_root - win_w.y();
                                let now_hit = hit_test_dot(
                                    wid_w.x(),
                                    wid_w.y(),
                                    wid_w.h(),
                                    mx_local,
                                    my_local,
                                )
                                .filter(|&idx| idx < 2 || allow_maximize);

                                if now_hit != s.hovered_dot {
                                    s.hovered_dot = now_hit;
                                    let keep = now_hit.is_some();
                                    s.watch_active = keep;
                                    drop(s);
                                    wid_w.redraw();
                                    if keep {
                                        app::repeat_timeout3(0.05, handle);
                                    }
                                } else if s.hovered_dot.is_some() {
                                    app::repeat_timeout3(0.05, handle);
                                } else {
                                    s.watch_active = false;
                                }
                            });
                        }
                    };

                match ev {
                    Event::Enter | Event::Move => {
                        let hit = active_hit(app::event_x(), app::event_y());
                        let changed = {
                            let mut s = st.borrow_mut();
                            if s.hovered_dot != hit {
                                s.hovered_dot = hit;
                                true
                            } else {
                                false
                            }
                        };
                        if changed {
                            w.redraw();
                        }
                        if hit.is_some() {
                            ensure_hover_watchdog(&st, w, &win_ctrl);
                        }
                        true
                    }
                    Event::Leave | Event::Unfocus | Event::Deactivate | Event::Hide => {
                        let mut s = st.borrow_mut();
                        if s.hovered_dot.is_some() || s.pressed_dot.is_some() {
                            s.hovered_dot = None;
                            s.pressed_dot = None;
                            drop(s);
                            w.redraw();
                        }
                        true
                    }
                    Event::Push => {
                        let hit = active_hit(app::event_x(), app::event_y());
                        {
                            let mut s = st.borrow_mut();
                            s.hovered_dot = hit;
                            s.pressed_dot = hit;
                        }
                        w.redraw();
                        if hit.is_some() {
                            ensure_hover_watchdog(&st, w, &win_ctrl);
                        }
                        true
                    }
                    Event::Drag => {
                        let hit = active_hit(app::event_x(), app::event_y());
                        let mut s = st.borrow_mut();
                        if s.hovered_dot != hit {
                            s.hovered_dot = hit;
                            drop(s);
                            w.redraw();
                        }
                        true
                    }
                    Event::Released => {
                        let hit = active_hit(app::event_x(), app::event_y());
                        let prev_pressed = {
                            let mut s = st.borrow_mut();
                            let prev = s.pressed_dot.take();
                            // Сразу «отжимаем» кнопку после клика, чтобы при сворачивании/разворачивании/закрытии
                            // эффект нажатия никогда не застревал!
                            s.hovered_dot = None;
                            prev
                        };
                        w.redraw();

                        if prev_pressed.is_some() && prev_pressed == hit {
                            match hit {
                                Some(0) => on_close(),
                                Some(1) => win_ctrl.iconize(),
                                Some(2) if allow_maximize => {
                                    if win_ctrl.maximize_active() {
                                        win_ctrl.un_maximize();
                                    } else {
                                        win_ctrl.maximize();
                                    }
                                }
                                _ => {}
                            }
                        }
                        true
                    }
                    _ => false,
                }
            });
        }

        // Отрисовка заголовка строго по оптическому центру шапки окна
        {
            let st = Rc::clone(&state);
            drag_widget.draw(move |w| {
                let s = st.borrow();
                draw::set_font(Font::Helvetica, 13);
                draw::set_draw_rgb_color(s.text_rgb.0, s.text_rgb.1, s.text_rgb.2);
                draw::push_clip(w.x() + 6, w.y(), (w.w() - 12).max(1), w.h());
                if let Some(p) = w.parent() {
                    draw::draw_text2(&s.title, p.x(), w.y(), p.w(), w.h(), Align::Center);
                } else {
                    draw::draw_text2(&s.title, w.x(), w.y(), w.w(), w.h(), Align::Center);
                }
                draw::pop_clip();
            });
        }

        // Перетаскивание окна с пропорциональной привязкой к курсору при «отрыве» из полноэкранного режима
        {
            let mut win_drag = win.clone();
            let drag_session = Rc::new(RefCell::new(DragSession::default()));
            drag_widget.handle(move |_w, ev| match ev {
                Event::Push => {
                    if allow_maximize && app::event_clicks() {
                        drag_session.borrow_mut().active = false;
                        if win_drag.maximize_active() {
                            win_drag.un_maximize();
                        } else {
                            win_drag.maximize();
                        }
                        return true;
                    }

                    let mx = app::event_x_root();
                    let my = app::event_y_root();
                    *drag_session.borrow_mut() = DragSession {
                        active: true,
                        start_root: (mx, my),
                        offset: (mx - win_drag.x(), my - win_drag.y()),
                    };
                    true
                }
                Event::Drag => {
                    let mut ds = drag_session.borrow_mut();
                    if !ds.active {
                        return true;
                    }

                    let mx = app::event_x_root();
                    let my = app::event_y_root();

                    if win_drag.maximize_active() {
                        // Не срываем развёрнутое окно от случайного микродвижения < 4px при обычном клике
                        if (mx - ds.start_root.0).abs() + (my - ds.start_root.1).abs() < 4 {
                            return true;
                        }

                        // Запоминаем относительную долю по ширине экрана, где курсор держит шапку
                        let max_w = win_drag.w().max(1) as f32;
                        let ratio = ((mx - win_drag.x()) as f32 / max_w).clamp(0.18, 0.86);
                        let grab_y = (my - win_drag.y()).clamp(6, TITLEBAR_HEIGHT - 6);

                        win_drag.un_maximize();

                        // Ставим восстановленное окно прямо под курсор мыши в ту же пропорциональную точку шапки
                        let restored_w = win_drag.w();
                        let min_dx = 200.min((restored_w / 2).max(80));
                        let max_dx = (restored_w - 36).max(min_dx);
                        let new_dx =
                            ((restored_w as f32 * ratio).round() as i32).clamp(min_dx, max_dx);

                        ds.offset = (new_dx, grab_y);
                        win_drag.set_pos(mx - new_dx, my - grab_y);
                        return true;
                    }

                    let (dx, dy) = ds.offset;
                    win_drag.set_pos(mx - dx, my - dy);
                    true
                }
                Event::Released => {
                    drag_session.borrow_mut().active = false;
                    true
                }
                _ => false,
            });
        }

        Self {
            bar,
            dots_widget,
            drag_widget,
            state,
        }
    }

    /// Обновляет текст заголовка в шапке окна.
    pub fn set_title(&mut self, title: &str) {
        self.state.borrow_mut().title = title.to_string();
        self.bar.redraw();
    }

    /// Обновляет цвета и скин шапки при переключении темы.
    pub fn apply_theme(&mut self, cfg: &EditorStyleConfig) {
        let chrome = cfg.chrome_palette();
        {
            let mut st = self.state.borrow_mut();
            st.panel_rgb = chrome.panel_bg;
            st.text_rgb = chrome.text_fg;
            st.is_dark = chrome.is_dark;
            st.ui_skin = cfg.ui_skin;
            st.dots_cache.clear();
        }
        self.dots_widget.redraw();
        self.drag_widget.redraw();
        self.bar.redraw();
    }
}

/// Создаёт компактный уголок изменения размера в правом нижнем углу безрамочного окна.
pub fn create_resize_grip(
    win: &Window,
    cfg: &EditorStyleConfig,
) -> (Widget, Rc<RefCell<(u8, u8, u8)>>) {
    let chrome = cfg.chrome_palette();
    let grip_color = Rc::new(RefCell::new(chrome.muted_fg));

    let mut grip = Widget::default();
    {
        let gc = Rc::clone(&grip_color);
        grip.draw(move |w| {
            let (r, g, b) = *gc.borrow();
            draw::set_draw_rgb_color(r, g, b);
            let bx = w.x() + w.w() - 5;
            let by = w.y() + w.h() - 5;
            for step in [0, 4, 8] {
                draw::draw_line(bx - step, by, bx, by - step);
            }
        });
    }

    {
        let mut win_rsz = win.clone();
        let start_geom = Rc::new(RefCell::new((0i32, 0i32, 0i32, 0i32)));
        grip.handle(move |_w, ev| match ev {
            Event::Enter => {
                win_rsz.set_cursor(Cursor::NWSE);
                true
            }
            Event::Leave => {
                win_rsz.set_cursor(Cursor::Default);
                true
            }
            Event::Push => {
                if !win_rsz.maximize_active() {
                    *start_geom.borrow_mut() = (
                        app::event_x_root(),
                        app::event_y_root(),
                        win_rsz.w(),
                        win_rsz.h(),
                    );
                }
                true
            }
            Event::Drag => {
                if !win_rsz.maximize_active() {
                    let (sx, sy, sw, sh) = *start_geom.borrow();
                    let nw = (sw + (app::event_x_root() - sx)).max(720);
                    let nh = (sh + (app::event_y_root() - sy)).max(460);
                    win_rsz.set_size(nw, nh);
                }
                true
            }
            _ => false,
        });
    }

    (grip, grip_color)
}

fn hit_test_dot(wx: i32, wy: i32, wh: i32, mx: i32, my: i32) -> Option<usize> {
    let cy = wy + wh / 2;
    let centers_x = [wx + 16, wx + 37, wx + 58];
    for (idx, &cx) in centers_x.iter().enumerate() {
        let dx = mx - cx;
        let dy = my - cy;
        if dx * dx + dy * dy <= 9 * 9 {
            return Some(idx);
        }
    }
    None
}

/// Рендерит 3 круглые точки macOS / Linux в физическом разрешении экрана (HiDPI 1:1, 16x MSAA)
/// с эффектом затемнения и лёгкого вдавливания конкретной кнопки при наведении/клике.
fn build_hidpi_traffic_lights(
    key: DotsCacheKey,
    dpi: f32,
    allow_maximize: bool,
) -> Option<RgbImage> {
    let phys_w = key.phys_w;
    let phys_h = key.phys_h;
    if phys_w <= 4 || phys_h <= 4 {
        return None;
    }

    let panel_rgb = key.panel_rgb;
    let is_dark = key.is_dark;
    let centers = [16.0 * dpi, 37.0 * dpi, 58.0 * dpi];
    let base_cy = (phys_h as f32 - 1.0) * 0.5;

    let colors: [(u8, u8, u8); 3] = [
        (255, 95, 86),  // Красный — закрыть
        (255, 189, 46), // Жёлтый — свернуть
        if allow_maximize {
            (40, 200, 64) // Зелёный — развернуть
        } else if is_dark {
            (72, 82, 102)
        } else {
            (188, 198, 212)
        },
    ];

    let mut rgb = Vec::with_capacity((phys_w * phys_h * 3) as usize);
    for py in 0..phys_h {
        let t = py as f32 / (phys_h - 1).max(1) as f32;
        let lift = if key.ui_skin == UiSkin::BasicFlat {
            0.0
        } else if is_dark {
            (1.0 - t) * 0.10
        } else {
            (1.0 - t) * 0.45
        };
        let bg_r = mix(panel_rgb.0, if is_dark { 210 } else { 255 }, lift);
        let bg_g = mix(panel_rgb.1, if is_dark { 228 } else { 255 }, lift);
        let bg_b = mix(panel_rgb.2, if is_dark { 255 } else { 255 }, lift);

        // Нижняя разделительная линия шапки (1 логический пиксель снизу)
        let is_sep_line = py >= phys_h - (dpi.round() as i32).max(1);
        let (base_r, base_g, base_b) = if is_sep_line {
            if is_dark {
                (
                    panel_rgb.0.saturating_add(34),
                    panel_rgb.1.saturating_add(42),
                    panel_rgb.2.saturating_add(60),
                )
            } else {
                (
                    panel_rgb.0.saturating_sub(24),
                    panel_rgb.1.saturating_sub(18),
                    panel_rgb.2.saturating_sub(8),
                )
            }
        } else {
            (bg_r, bg_g, bg_b)
        };

        for px in 0..phys_w {
            let mut out = (base_r, base_g, base_b);
            let xf = px as f32;
            let yf = py as f32;

            for (idx, &cx) in centers.iter().enumerate() {
                let is_pressed = key.pressed_dot == Some(idx);
                let is_hovered = key.hovered_dot == Some(idx);

                // При наведении кружок чуть утапливается вниз и слегка сжимается («прожимается»)
                let (cy, radius) = if is_pressed {
                    (base_cy + 1.10 * dpi, 5.80 * dpi)
                } else if is_hovered {
                    (base_cy + 0.65 * dpi, 5.98 * dpi)
                } else {
                    (base_cy, 6.20 * dpi)
                };
                let r_sq = radius * radius;

                let dx = xf - cx;
                let dy = yf - cy;
                let dist = (dx * dx + dy * dy).sqrt() - radius;

                if dist < 1.1 {
                    let cov = if dist < -1.1 {
                        1.0
                    } else {
                        let mut inside = 0;
                        for sy in 0..4 {
                            let oy = dy - 0.375 + (sy as f32) * 0.25;
                            for sx in 0..4 {
                                let ox = dx - 0.375 + (sx as f32) * 0.25;
                                if ox * ox + oy * oy <= r_sq {
                                    inside += 1;
                                }
                            }
                        }
                        inside as f32 / 16.0
                    };

                    if cov > 0.0 {
                        let (mut cr, mut cg, mut cb) = colors[idx];

                        if is_pressed {
                            // Глубокое затемнение и внутренняя верхняя тень при зажатии кнопки
                            cr = mix(cr, 12, 0.38);
                            cg = mix(cg, 12, 0.38);
                            cb = mix(cb, 16, 0.38);
                            if dy < 0.0 && dist > -2.0 * dpi {
                                let inner_sh = ((-dy / radius)
                                    * ((dist + 2.0 * dpi) / (2.0 * dpi)))
                                    .clamp(0.0, 1.0)
                                    * 0.35;
                                cr = mix(cr, 0, inner_sh);
                                cg = mix(cg, 0, inner_sh);
                                cb = mix(cb, 0, inner_sh);
                            }
                        } else if is_hovered {
                            // Мягкое затемнение и ощущение лёгкого «вдавливания» при наведении мыши
                            cr = mix(cr, 16, 0.24);
                            cg = mix(cg, 16, 0.24);
                            cb = mix(cb, 22, 0.24);
                            if dy < 0.0 && dist > -1.7 * dpi {
                                let inner_sh = ((-dy / radius)
                                    * ((dist + 1.7 * dpi) / (1.7 * dpi)))
                                    .clamp(0.0, 1.0)
                                    * 0.25;
                                cr = mix(cr, 8, inner_sh);
                                cg = mix(cg, 8, inner_sh);
                                cb = mix(cb, 12, inner_sh);
                            }
                        } else {
                            // В покое — мягкий объёмный блик сверху кружка
                            let spec =
                                ((-dy / radius) * 0.65 + 0.25).clamp(0.0, 1.0).powi(2) * 0.16;
                            cr = mix(cr, 255, spec);
                            cg = mix(cg, 255, spec);
                            cb = mix(cb, 255, spec);
                        }

                        // Чёткая тонкая окантовка кружка
                        if dist > -0.95 * dpi {
                            let rim_alpha = if is_pressed || is_hovered { 0.30 } else { 0.18 };
                            let rim =
                                ((dist + 0.95 * dpi) / (0.95 * dpi)).clamp(0.0, 1.0) * rim_alpha;
                            cr = mix(cr, 12, rim);
                            cg = mix(cg, 12, rim);
                            cb = mix(cb, 18, rim);
                        }

                        out = (
                            mix(out.0, cr, cov),
                            mix(out.1, cg, cov),
                            mix(out.2, cb, cov),
                        );
                    }
                }
            }

            rgb.push(out.0);
            rgb.push(out.1);
            rgb.push(out.2);
        }
    }

    RgbImage::new(&rgb, phys_w, phys_h, ColorDepth::Rgb8).ok()
}

#[inline]
fn mix(a: u8, b: u8, t: f32) -> u8 {
    let t = t.clamp(0.0, 1.0);
    (a as f32 * (1.0 - t) + b as f32 * t)
        .round()
        .clamp(0.0, 255.0) as u8
}
