use crate::{
    model::{
        lexer::TokenCategory,
        style_config::{EditorStyleConfig, FontFamily, FontSlantWeight, ThemePreset, UiSkin},
    },
    ui::{
        editor_tab::{AppMsg, resolve_fltk_font},
        theme::{
            configure_frameless_os_window, draw_frameless_window_border, style_button,
            style_choice, style_slider,
        },
        titlebar::{CustomTitleBar, TITLEBAR_HEIGHT},
    },
};
use fltk::{
    app,
    button::Button,
    enums::{Align, Color, Event, Font, FrameType},
    frame::Frame,
    group::{Flex, FlexType, Group},
    menu::Choice,
    prelude::*,
    valuator::HorValueSlider,
    window::Window,
};
use std::{cell::RefCell, rc::Rc};

/// Виджеты одной строки настройки категории токена в окне «Редактор стилей».
struct TokenRowWidgets {
    category: TokenCategory,
    preview_box: Frame,
    name_lbl: Frame,
    color_btn: Button,
    weight_choice: Choice,
}

/// Немодальное безрамочное окно «Настройки стилей и шрифтов» с 3 точками macOS/Linux.
pub struct StyleDialog {
    window: Window,
    titlebar: CustomTitleBar,
    live_cfg: Rc<RefCell<EditorStyleConfig>>,
    preset_choice: Choice,
    skin_choice: Choice,
    font_choice: Choice,
    size_slider: HorValueSlider,
    bg_preview: Frame,
    bg_btn: Button,
    btn_done: Button,
    hint_lbl: Frame,
    static_labels: Vec<Frame>,
    rows: Vec<TokenRowWidgets>,
}

impl StyleDialog {
    /// Создаёт независимое безрамочное окно настройки тем, UI-скина, шрифтов и синтаксических стилей.
    pub fn new(initial_cfg: &EditorStyleConfig, sender: app::Sender<AppMsg>) -> Self {
        Group::set_current(None::<&Group>);

        let mut window = Window::default()
            .with_size(905, 555)
            .with_label("Настройки стилей, тем и шрифтов")
            .center_screen();
        window.set_border(false);
        window.make_resizable(false);

        let live_cfg = Rc::new(RefCell::new(initial_cfg.clone()));
        let mut static_labels: Vec<Frame> = Vec::new();

        let mut outer = Flex::default_fill().with_type(FlexType::Column);
        outer.set_margins(1, 1, 1, 1);
        outer.set_pad(0);

        let titlebar = CustomTitleBar::new(
            &window,
            "Настройки стилей, тем и шрифтов (Style Editor)",
            initial_cfg,
            false,
            move || {
                sender.send(AppMsg::CloseStyleDialog);
            },
            |_bar| {},
        );
        outer.fixed(&titlebar.bar, TITLEBAR_HEIGHT);

        let mut root = Flex::default().with_type(FlexType::Column);
        root.set_margins(16, 12, 16, 12);
        root.set_pad(10);

        // 1. Выбор цветовой темы (8 пресетов) и визуального стиля интерфейса (Liquid Glass и др.)
        let mut theme_row = Flex::default().with_type(FlexType::Row);
        theme_row.set_pad(8);

        let mut preset_lbl = Frame::default().with_label("Цветовая тема:");
        preset_lbl.set_align(Align::Left | Align::Inside);
        preset_lbl.set_label_font(Font::HelveticaBold);
        theme_row.fixed(&preset_lbl, 128);
        static_labels.push(preset_lbl);

        let mut preset_choice = Choice::default();
        for p in ThemePreset::ALL {
            preset_choice.add_choice(p.display_name());
        }
        preset_choice.add_choice("Пользовательская (своя палитра)");
        preset_choice.emit(sender, AppMsg::StylePresetChoiceChanged);
        theme_row.fixed(&preset_choice, 315);

        let mut skin_lbl = Frame::default().with_label("Стиль UI:");
        skin_lbl.set_align(Align::Right | Align::Inside);
        skin_lbl.set_label_font(Font::HelveticaBold);
        theme_row.fixed(&skin_lbl, 86);
        static_labels.push(skin_lbl);

        let mut skin_choice = Choice::default();
        for skin in UiSkin::ALL {
            skin_choice.add_choice(skin.display_name());
        }
        skin_choice.emit(sender, AppMsg::StyleSkinChanged);

        theme_row.end();
        root.fixed(&theme_row, 30);

        // 2. Базовое семейство шрифта и размер (кегль)
        let mut font_row = Flex::default().with_type(FlexType::Row);
        font_row.set_pad(8);

        let mut font_lbl = Frame::default().with_label("Шрифт кода:");
        font_lbl.set_align(Align::Left | Align::Inside);
        font_row.fixed(&font_lbl, 128);
        static_labels.push(font_lbl);

        let mut font_choice = Choice::default();
        for fam in FontFamily::ALL {
            font_choice.add_choice(fam.display_name());
        }
        font_choice.emit(sender, AppMsg::StyleFontChanged);
        font_row.fixed(&font_choice, 315);

        let mut size_lbl = Frame::default().with_label("Кегль:");
        size_lbl.set_align(Align::Right | Align::Inside);
        font_row.fixed(&size_lbl, 86);
        static_labels.push(size_lbl);

        let mut size_slider = HorValueSlider::default();
        size_slider.set_bounds(10.0, 28.0);
        size_slider.set_step(1.0, 1);
        size_slider.set_precision(0);
        size_slider.emit(sender, AppMsg::StyleFontSizeChanged);

        font_row.end();
        root.fixed(&font_row, 30);

        // 3. Строка выбора фона редактора
        let mut bg_row = Flex::default().with_type(FlexType::Row);
        bg_row.set_pad(8);

        let mut bg_lbl = Frame::default().with_label("Фон редактора:");
        bg_lbl.set_align(Align::Left | Align::Inside);
        bg_row.fixed(&bg_lbl, 128);
        static_labels.push(bg_lbl);

        let mut bg_preview = Frame::default();
        bg_preview.set_frame(FrameType::BorderBox);
        bg_row.fixed(&bg_preview, 52);

        let mut bg_btn = Button::default();
        bg_btn.emit(sender, AppMsg::StylePickBackground);
        bg_row.fixed(&bg_btn, 165);

        let _bg_spacer = Frame::default();
        bg_row.end();
        root.fixed(&bg_row, 30);

        // 4. Заголовок таблицы категорий токенов
        let mut header_row = Flex::default().with_type(FlexType::Row);
        header_row.set_pad(8);

        let mut h_sample = Frame::default().with_label("Вид");
        h_sample.set_label_font(Font::HelveticaBold);
        header_row.fixed(&h_sample, 54);
        static_labels.push(h_sample);

        let mut h_cat = Frame::default().with_label("Категория синтаксических токенов");
        h_cat.set_align(Align::Left | Align::Inside);
        h_cat.set_label_font(Font::HelveticaBold);
        static_labels.push(h_cat);

        let mut h_col = Frame::default().with_label("Цвет (RGB)");
        h_col.set_label_font(Font::HelveticaBold);
        header_row.fixed(&h_col, 125);
        static_labels.push(h_col);

        let mut h_w = Frame::default().with_label("Начертание шрифта");
        h_w.set_label_font(Font::HelveticaBold);
        header_row.fixed(&h_w, 205);
        static_labels.push(h_w);

        header_row.end();
        root.fixed(&header_row, 24);

        // 5. Строки настройки 6 категорий токенов ('A'..'F')
        let mut rows = Vec::with_capacity(TokenCategory::ALL.len());
        for category in TokenCategory::ALL {
            let mut row = Flex::default().with_type(FlexType::Row);
            row.set_pad(8);

            let mut preview_box = Frame::default().with_label("Aa");
            preview_box.set_frame(FrameType::BorderBox);
            row.fixed(&preview_box, 54);

            let mut name_lbl = Frame::default().with_label(category.display_name());
            name_lbl.set_align(Align::Left | Align::Inside);

            let mut color_btn = Button::default();
            color_btn.emit(sender, AppMsg::StylePickTokenColor(category));
            row.fixed(&color_btn, 125);

            let mut weight_choice = Choice::default();
            for w in FontSlantWeight::ALL {
                weight_choice.add_choice(w.display_name());
            }
            weight_choice.emit(sender, AppMsg::StyleWeightChanged(category));
            row.fixed(&weight_choice, 205);

            row.end();
            root.fixed(&row, 32);

            rows.push(TokenRowWidgets {
                category,
                preview_box,
                name_lbl,
                color_btn,
                weight_choice,
            });
        }

        let _flex_filler = Frame::default();

        // 6. Нижняя панель с автосохранением и кнопкой закрытия
        let mut footer = Flex::default().with_type(FlexType::Row);
        footer.set_pad(8);

        let mut hint_lbl = Frame::default().with_label(
            "Все изменения сразу применяются ко всему редактору и сохраняются в editor_styles.json",
        );
        hint_lbl.set_align(Align::Left | Align::Inside);
        hint_lbl.set_label_size(12);

        let mut btn_done = Button::default().with_label("Готово");
        btn_done.emit(sender, AppMsg::CloseStyleDialog);
        footer.fixed(&btn_done, 125);

        footer.end();
        root.fixed(&footer, 34);

        root.end();
        outer.end();
        window.end();

        {
            let cfg_ref = Rc::clone(&live_cfg);
            window.draw(move |w| {
                w.draw_children();
                draw_frameless_window_border(w.w(), w.h(), &cfg_ref.borrow());
            });
        }

        window.set_callback(move |_| {
            if app::event() == Event::Close {
                sender.send(AppMsg::CloseStyleDialog);
            }
        });

        let mut dlg = Self {
            window,
            titlebar,
            live_cfg,
            preset_choice,
            skin_choice,
            font_choice,
            size_slider,
            bg_preview,
            bg_btn,
            btn_done,
            hint_lbl,
            static_labels,
            rows,
        };
        dlg.sync_from_config(initial_cfg);
        dlg
    }

    /// Открывает и выводит на передний план безрамочное окно настройки стилей.
    pub fn show(&mut self, cfg: &EditorStyleConfig) {
        self.sync_from_config(cfg);
        self.window.show();
        configure_frameless_os_window(&self.window, false);
    }

    /// Скрывает окно настройки стилей.
    pub fn hide(&mut self) {
        self.window.hide();
    }

    pub fn selected_preset(&self) -> Option<ThemePreset> {
        ThemePreset::from_index(self.preset_choice.value())
    }

    pub fn selected_ui_skin(&self) -> UiSkin {
        UiSkin::from_index(self.skin_choice.value())
    }

    pub fn selected_font_family(&self) -> FontFamily {
        FontFamily::from_index(self.font_choice.value())
    }

    pub fn selected_font_size(&self) -> i32 {
        (self.size_slider.value().round() as i32).clamp(10, 28)
    }

    pub fn selected_weight(&self, category: TokenCategory) -> Option<FontSlantWeight> {
        self.rows
            .iter()
            .find(|r| r.category == category)
            .map(|r| FontSlantWeight::from_index(r.weight_choice.value()))
    }

    /// Полностью синхронизирует значения И визуальное оформление самого окна настроек под активную тему.
    pub fn sync_from_config(&mut self, cfg: &EditorStyleConfig) {
        *self.live_cfg.borrow_mut() = cfg.clone();
        self.titlebar.apply_theme(cfg);

        let chrome = cfg.chrome_palette();
        let panel_col = Color::from_rgb(chrome.panel_bg.0, chrome.panel_bg.1, chrome.panel_bg.2);
        let text_col = Color::from_rgb(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);
        let muted_col = Color::from_rgb(chrome.muted_fg.0, chrome.muted_fg.1, chrome.muted_fg.2);

        self.window.set_color(panel_col);

        for lbl in &mut self.static_labels {
            lbl.set_label_color(text_col);
        }
        self.hint_lbl.set_label_color(muted_col);

        let preset_idx = cfg
            .matching_preset_index()
            .unwrap_or(ThemePreset::ALL.len() as i32);
        self.preset_choice.set_value(preset_idx);
        style_choice(&mut self.preset_choice, cfg);

        self.skin_choice.set_value(cfg.ui_skin.to_index());
        style_choice(&mut self.skin_choice, cfg);

        self.font_choice.set_value(cfg.font_family.to_index());
        style_choice(&mut self.font_choice, cfg);

        self.size_slider.set_value(cfg.font_size as f64);
        style_slider(&mut self.size_slider, cfg);

        let (bg_r, bg_g, bg_b) = cfg.background_rgb;
        let bg_color = Color::from_rgb(bg_r, bg_g, bg_b);
        self.bg_preview.set_color(bg_color);
        self.bg_preview.redraw();

        self.bg_btn
            .set_label(&format!("Палитра #{:02X}{:02X}{:02X}", bg_r, bg_g, bg_b));
        style_button(&mut self.bg_btn, cfg);

        style_button(&mut self.btn_done, cfg);

        for row in &mut self.rows {
            let rule = cfg.rule(row.category);
            let (r, g, b) = rule.rgb;

            row.name_lbl.set_label_color(text_col);

            row.preview_box.set_color(bg_color);
            row.preview_box.set_label_color(Color::from_rgb(r, g, b));
            row.preview_box
                .set_label_font(resolve_fltk_font(cfg.font_family, rule.weight));
            row.preview_box.set_label_size(cfg.font_size.clamp(11, 20));
            row.preview_box.redraw();

            row.color_btn
                .set_label(&format!("#{:02X}{:02X}{:02X}", r, g, b));
            style_button(&mut row.color_btn, cfg);

            row.weight_choice.set_value(rule.weight.to_index());
            style_choice(&mut row.weight_choice, cfg);
        }

        self.window.redraw();
    }
}
