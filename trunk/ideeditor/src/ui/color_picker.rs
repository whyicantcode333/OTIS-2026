use crate::{
    model::style_config::EditorStyleConfig,
    ui::{
        theme::{
            configure_frameless_os_window, draw_frameless_window_border, style_button,
            style_slider_accent,
        },
        titlebar::{CustomTitleBar, TITLEBAR_HEIGHT},
    },
};
use fltk::{
    app,
    button::Button,
    draw,
    enums::{Align, CallbackTrigger, Color, Event, Font, FrameType},
    frame::Frame,
    group::{Flex, FlexType, Group},
    image::RgbImage,
    input::Input,
    prelude::*,
    valuator::HorValueSlider,
    widget::Widget,
    window::Window,
};
use std::{cell::RefCell, rc::Rc};

/// Курируемые быстрые образцы цветов для синтаксиса и фона.
const QUICK_SWATCHES: [(u8, u8, u8); 12] = [
    (192, 132, 252), // Лаванда / Фиолетовый
    (255, 121, 198), // Розовый Dracula
    (103, 232, 249), // Ледяной циан Liquid Glass
    (56, 189, 248),  // Небесно-голубой
    (110, 231, 183), // Изумрудно-мятный
    (166, 227, 161), // Пастельно-зелёный
    (251, 191, 36),  // Янтарно-золотой
    (250, 140, 100), // Кораллово-оранжевый
    (244, 63, 94),   // Рубиново-красный
    (148, 163, 184), // Сланцево-серый (комментарии)
    (245, 250, 255), // Светлый ледяной фон
    (16, 24, 41),    // Полуночный тёмный фон
];

/// Открывает собственное безрамочное окно выбора цвета с 3 точками macOS/Linux,
/// 2D-спектром оттенков, ползунками R/G/B, вводом HEX и быстрой палитрой.
pub fn pick_color_dialog(
    title: &str,
    initial: (u8, u8, u8),
    cfg: &EditorStyleConfig,
) -> Option<(u8, u8, u8)> {
    Group::set_current(None::<&Group>);

    let chrome = cfg.chrome_palette();
    let panel_col = Color::from_rgb(chrome.panel_bg.0, chrome.panel_bg.1, chrome.panel_bg.2);
    let text_col = Color::from_rgb(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);
    let surface_col = Color::from_rgb(
        chrome.surface_bg.0,
        chrome.surface_bg.1,
        chrome.surface_bg.2,
    );

    let mut win = Window::default()
        .with_size(548, 430)
        .with_label(title)
        .center_screen();
    win.set_border(false);
    win.set_color(panel_col);
    win.make_modal(true);

    let state = Rc::new(RefCell::new((initial, false)));

    let mut outer = Flex::default_fill().with_type(FlexType::Column);
    outer.set_margins(1, 1, 1, 1);
    outer.set_pad(0);

    // Кастомная безрамочная шапка с 3 точками
    let mut win_close = win.clone();
    let titlebar = CustomTitleBar::new(
        &win,
        title,
        cfg,
        false,
        move || {
            win_close.hide();
        },
        |_bar| {},
    );
    outer.fixed(&titlebar.bar, TITLEBAR_HEIGHT);

    let mut root = Flex::default().with_type(FlexType::Column);
    root.set_margins(14, 12, 14, 12);
    root.set_pad(10);

    let mut main_area = Flex::default().with_type(FlexType::Row);
    main_area.set_pad(14);

    let mut spectrum_widget = Widget::default();
    main_area.fixed(&spectrum_widget, 205);

    let mut right_col = Flex::default().with_type(FlexType::Column);
    right_col.set_pad(8);

    // Карточки сравнения «Текущий» и «Новый»
    let mut preview_row = Flex::default().with_type(FlexType::Row);
    preview_row.set_pad(8);

    let mut old_box = Frame::default().with_label("Текущий");
    old_box.set_frame(FrameType::BorderBox);
    old_box.set_color(Color::from_rgb(initial.0, initial.1, initial.2));
    old_box.set_label_color(contrast_text_color(initial));

    let mut new_box = Frame::default().with_label("Новый");
    new_box.set_frame(FrameType::BorderBox);
    new_box.set_color(Color::from_rgb(initial.0, initial.1, initial.2));
    new_box.set_label_color(contrast_text_color(initial));

    preview_row.end();
    right_col.fixed(&preview_row, 44);

    // Просторные ползунки R, G, B (0..255) с индивидуальной цветовой шкалой
    let mut r_slider = make_channel_row(
        &mut right_col,
        "Красный (R):",
        initial.0,
        text_col,
        cfg,
        (239, 68, 68),
    );
    let mut g_slider = make_channel_row(
        &mut right_col,
        "Зелёный (G):",
        initial.1,
        text_col,
        cfg,
        (34, 197, 94),
    );
    let mut b_slider = make_channel_row(
        &mut right_col,
        "Синий (B):",
        initial.2,
        text_col,
        cfg,
        (59, 130, 246),
    );

    // Поле прямого ввода HEX-кода (#RRGGBB)
    let mut hex_row = Flex::default().with_type(FlexType::Row);
    hex_row.set_pad(8);
    let mut hex_lbl = Frame::default().with_label("HEX-код:");
    hex_lbl.set_align(Align::Left | Align::Inside);
    hex_lbl.set_label_color(text_col);
    hex_row.fixed(&hex_lbl, 95);

    let mut hex_input = Input::default();
    hex_input.set_color(surface_col);
    hex_input.set_text_color(text_col);
    hex_input.set_text_font(Font::CourierBold);
    hex_input.set_value(&format!(
        "#{:02X}{:02X}{:02X}",
        initial.0, initial.1, initial.2
    ));
    hex_input.set_trigger(CallbackTrigger::Changed);
    hex_row.end();
    right_col.fixed(&hex_row, 30);

    right_col.end();
    main_area.end();

    // Полоса из 12 быстрых цветовых образцов
    let mut swatch_lbl = Frame::default().with_label("Быстрая палитра оттенков:");
    swatch_lbl.set_align(Align::Left | Align::Inside);
    swatch_lbl.set_label_color(text_col);
    swatch_lbl.set_label_size(12);
    root.fixed(&swatch_lbl, 18);

    let mut swatch_row = Flex::default().with_type(FlexType::Row);
    swatch_row.set_pad(6);
    let mut swatch_btns: Vec<(Button, (u8, u8, u8))> = Vec::with_capacity(QUICK_SWATCHES.len());
    for &(sr, sg, sb) in &QUICK_SWATCHES {
        let mut sw = Button::default();
        sw.set_frame(FrameType::BorderBox);
        sw.set_color(Color::from_rgb(sr, sg, sb));
        sw.set_selection_color(Color::from_rgb(sr, sg, sb));
        swatch_btns.push((sw, (sr, sg, sb)));
    }
    swatch_row.end();
    root.fixed(&swatch_row, 28);

    // Нижние кнопки «Применить» и «Отмена»
    let mut footer = Flex::default().with_type(FlexType::Row);
    footer.set_pad(10);
    let _spacer = Frame::default();

    let mut btn_ok = Button::default().with_label("Применить");
    style_button(&mut btn_ok, cfg);
    footer.fixed(&btn_ok, 132);

    let mut btn_cancel = Button::default().with_label("Отмена");
    style_button(&mut btn_cancel, cfg);
    footer.fixed(&btn_cancel, 122);

    footer.end();
    root.fixed(&footer, 34);

    root.end();
    outer.end();
    win.end();

    let cfg_border = cfg.clone();
    win.draw(move |w| {
        w.draw_children();
        draw_frameless_window_border(w.w(), w.h(), &cfg_border);
    });

    let spec_w = 205;
    let spec_h = 215;
    let mut rgb_bytes = Vec::with_capacity((spec_w * spec_h * 3) as usize);
    for py in 0..spec_h {
        let val_v = 1.0 - (py as f32 / (spec_h - 1).max(1) as f32) * 0.88;
        for px in 0..spec_w {
            let hue = (px as f32 / (spec_w - 1).max(1) as f32) * 360.0;
            let (rr, gg, bb) = hsv_to_rgb(hue, 0.85, val_v);
            rgb_bytes.push(rr);
            rgb_bytes.push(gg);
            rgb_bytes.push(bb);
        }
    }
    let spec_img = RgbImage::new(&rgb_bytes, spec_w, spec_h, fltk::enums::ColorDepth::Rgb8).ok();

    spectrum_widget.draw(move |w| {
        if let Some(mut img) = spec_img.clone() {
            img.scale(w.w(), w.h(), false, true);
            img.draw(w.x(), w.y(), w.w(), w.h());
        }
        draw::set_draw_rgb_color(120, 135, 160);
        draw::draw_rect(w.x(), w.y(), w.w(), w.h());
    });

    let sync_ui = {
        let state = Rc::clone(&state);
        let new_box = new_box.clone();
        let r_slider = r_slider.clone();
        let g_slider = g_slider.clone();
        let b_slider = b_slider.clone();
        let hex_input = hex_input.clone();
        Rc::new(move |rgb: (u8, u8, u8), update_hex: bool| {
            state.borrow_mut().0 = rgb;
            let mut nb = new_box.clone();
            nb.set_color(Color::from_rgb(rgb.0, rgb.1, rgb.2));
            nb.set_label_color(contrast_text_color(rgb));
            nb.redraw();

            let mut rs = r_slider.clone();
            let mut gs = g_slider.clone();
            let mut bs = b_slider.clone();
            rs.set_value(rgb.0 as f64);
            gs.set_value(rgb.1 as f64);
            bs.set_value(rgb.2 as f64);
            rs.redraw();
            gs.redraw();
            bs.redraw();

            if update_hex {
                let mut hi = hex_input.clone();
                hi.set_value(&format!("#{:02X}{:02X}{:02X}", rgb.0, rgb.1, rgb.2));
            }
        })
    };

    {
        let sync = Rc::clone(&sync_ui);
        spectrum_widget.handle(move |w, ev| match ev {
            Event::Push | Event::Drag => {
                let mx = (app::event_x() - w.x()).clamp(0, (w.w() - 1).max(1));
                let my = (app::event_y() - w.y()).clamp(0, (w.h() - 1).max(1));
                let hue = (mx as f32 / (w.w() - 1).max(1) as f32) * 360.0;
                let val_v = 1.0 - (my as f32 / (w.h() - 1).max(1) as f32) * 0.88;
                let picked = hsv_to_rgb(hue, 0.85, val_v);
                sync(picked, true);
                true
            }
            _ => false,
        });
    }

    {
        let sync = Rc::clone(&sync_ui);
        let gs = g_slider.clone();
        let bs = b_slider.clone();
        r_slider.set_callback(move |rs| {
            sync(
                (
                    rs.value().round() as u8,
                    gs.value().round() as u8,
                    bs.value().round() as u8,
                ),
                true,
            );
        });
    }
    {
        let sync = Rc::clone(&sync_ui);
        let rs = r_slider.clone();
        let bs = b_slider.clone();
        g_slider.set_callback(move |gs| {
            sync(
                (
                    rs.value().round() as u8,
                    gs.value().round() as u8,
                    bs.value().round() as u8,
                ),
                true,
            );
        });
    }
    {
        let sync = Rc::clone(&sync_ui);
        let rs = r_slider.clone();
        let gs = g_slider.clone();
        b_slider.set_callback(move |bs| {
            sync(
                (
                    rs.value().round() as u8,
                    gs.value().round() as u8,
                    bs.value().round() as u8,
                ),
                true,
            );
        });
    }

    {
        let sync = Rc::clone(&sync_ui);
        hex_input.set_callback(move |inp| {
            if let Some(rgb) = parse_hex_rgb(&inp.value()) {
                sync(rgb, false);
            }
        });
    }

    for (mut sw_btn, rgb) in swatch_btns {
        let sync = Rc::clone(&sync_ui);
        sw_btn.set_callback(move |_| {
            sync(rgb, true);
        });
    }

    {
        let state_ok = Rc::clone(&state);
        let mut win_ok = win.clone();
        btn_ok.set_callback(move |_| {
            state_ok.borrow_mut().1 = true;
            win_ok.hide();
        });
    }
    {
        let mut win_cancel = win.clone();
        btn_cancel.set_callback(move |_| {
            win_cancel.hide();
        });
    }

    win.show();
    configure_frameless_os_window(&win, false);

    while win.shown() {
        app::wait();
    }

    let (chosen_rgb, accepted) = *state.borrow();
    if accepted { Some(chosen_rgb) } else { None }
}

/// Безрамочное диалоговое окно подтверждения несохранённых изменений в едином стиле редактора.
/// Возвращает `Some(0)` = Сохранить, `Some(1)` = Не сохранять, `None` = Отмена.
pub fn confirm_unsaved_dialog(doc_name: &str, cfg: &EditorStyleConfig) -> Option<usize> {
    Group::set_current(None::<&Group>);

    let chrome = cfg.chrome_palette();
    let panel_col = Color::from_rgb(chrome.panel_bg.0, chrome.panel_bg.1, chrome.panel_bg.2);
    let text_col = Color::from_rgb(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);

    let mut win = Window::default()
        .with_size(460, 195)
        .with_label("Несохранённые изменения")
        .center_screen();
    win.set_border(false);
    win.set_color(panel_col);
    win.make_modal(true);

    let result: Rc<RefCell<Option<usize>>> = Rc::new(RefCell::new(None));

    let mut outer = Flex::default_fill().with_type(FlexType::Column);
    outer.set_margins(1, 1, 1, 1);
    outer.set_pad(0);

    let mut win_close = win.clone();
    let titlebar = CustomTitleBar::new(
        &win,
        "Несохранённые изменения",
        cfg,
        false,
        move || {
            win_close.hide();
        },
        |_bar| {},
    );
    outer.fixed(&titlebar.bar, TITLEBAR_HEIGHT);

    let mut body = Flex::default().with_type(FlexType::Column);
    body.set_margins(18, 14, 18, 14);
    body.set_pad(14);

    let mut msg_lbl = Frame::default().with_label(&format!(
        "В документе «{}» есть несохранённые изменения.\nСохранить файл перед закрытием?",
        doc_name
    ));
    msg_lbl.set_align(Align::Left | Align::Inside | Align::Wrap);
    msg_lbl.set_label_color(text_col);

    let mut btn_row = Flex::default().with_type(FlexType::Row);
    btn_row.set_pad(10);
    let _sp = Frame::default();

    let mut btn_save = Button::default().with_label("Сохранить");
    style_button(&mut btn_save, cfg);
    btn_row.fixed(&btn_save, 125);

    let mut btn_discard = Button::default().with_label("Не сохранять");
    style_button(&mut btn_discard, cfg);
    btn_row.fixed(&btn_discard, 135);

    let mut btn_cancel = Button::default().with_label("Отмена");
    style_button(&mut btn_cancel, cfg);
    btn_row.fixed(&btn_cancel, 110);

    btn_row.end();
    body.fixed(&btn_row, 34);

    body.end();
    outer.end();
    win.end();

    let cfg_border = cfg.clone();
    win.draw(move |w| {
        w.draw_children();
        draw_frameless_window_border(w.w(), w.h(), &cfg_border);
    });

    {
        let res = Rc::clone(&result);
        let mut w = win.clone();
        btn_save.set_callback(move |_| {
            *res.borrow_mut() = Some(0);
            w.hide();
        });
    }
    {
        let res = Rc::clone(&result);
        let mut w = win.clone();
        btn_discard.set_callback(move |_| {
            *res.borrow_mut() = Some(1);
            w.hide();
        });
    }
    {
        let mut w = win.clone();
        btn_cancel.set_callback(move |_| {
            w.hide();
        });
    }

    win.show();
    configure_frameless_os_window(&win, false);

    while win.shown() {
        app::wait();
    }

    let picked = *result.borrow();
    picked
}

fn make_channel_row(
    parent: &mut Flex,
    label_txt: &str,
    initial_val: u8,
    text_col: Color,
    cfg: &EditorStyleConfig,
    accent_rgb: (u8, u8, u8),
) -> HorValueSlider {
    let mut row = Flex::default().with_type(FlexType::Row);
    row.set_pad(8);

    let mut lbl = Frame::default().with_label(label_txt);
    lbl.set_align(Align::Left | Align::Inside);
    lbl.set_label_color(text_col);
    row.fixed(&lbl, 95);

    let mut slider = HorValueSlider::default();
    slider.set_bounds(0.0, 255.0);
    slider.set_step(1.0, 1);
    slider.set_precision(0);
    slider.set_value(initial_val as f64);
    style_slider_accent(&mut slider, cfg, Some(accent_rgb));

    row.end();
    parent.fixed(&row, 28);
    slider
}

fn contrast_text_color(rgb: (u8, u8, u8)) -> Color {
    let lum = (rgb.0 as u32 * 299 + rgb.1 as u32 * 587 + rgb.2 as u32 * 114) / 1000;
    if lum < 135 {
        Color::from_rgb(241, 245, 249)
    } else {
        Color::from_rgb(15, 23, 42)
    }
}

fn parse_hex_rgb(raw: &str) -> Option<(u8, u8, u8)> {
    let trimmed = raw.trim().trim_start_matches('#');
    if trimmed.len() != 6 {
        return None;
    }
    let r = u8::from_str_radix(&trimmed[0..2], 16).ok()?;
    let g = u8::from_str_radix(&trimmed[2..4], 16).ok()?;
    let b = u8::from_str_radix(&trimmed[4..6], 16).ok()?;
    Some((r, g, b))
}

fn hsv_to_rgb(h: f32, s: f32, v: f32) -> (u8, u8, u8) {
    let c = v * s;
    let hp = (h / 60.0) % 6.0;
    let x = c * (1.0 - ((hp % 2.0) - 1.0).abs());
    let (r1, g1, b1) = if hp < 1.0 {
        (c, x, 0.0)
    } else if hp < 2.0 {
        (x, c, 0.0)
    } else if hp < 3.0 {
        (0.0, c, x)
    } else if hp < 4.0 {
        (0.0, x, c)
    } else if hp < 5.0 {
        (x, 0.0, c)
    } else {
        (c, 0.0, x)
    };
    let m = v - c;
    (
        ((r1 + m) * 255.0).round().clamp(0.0, 255.0) as u8,
        ((g1 + m) * 255.0).round().clamp(0.0, 255.0) as u8,
        ((b1 + m) * 255.0).round().clamp(0.0, 255.0) as u8,
    )
}
