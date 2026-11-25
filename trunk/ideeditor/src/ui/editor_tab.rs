use crate::model::{
    lexer::{TokenCategory, highlight_rust_to_style_string},
    style_config::{EditorStyleConfig, FontFamily, FontSlantWeight, ThemePreset},
};
use fltk::{
    app,
    enums::{Align, Color, Event, Font, FrameType},
    group::{Flex, FlexType},
    prelude::*,
    text::{StyleTableEntry, TextBuffer, TextEditor, WrapMode},
};

/// Сообщения графического интерфейса (передаются через канал `app::channel`).
#[derive(Debug, Clone, Copy)]
pub enum AppMsg {
    FileNew,
    FileOpen,
    FileSave,
    FileSaveAs,
    CloseActiveTab,
    AppExit,
    TabSwitched,
    BufferChanged(usize),
    HighlightReady,
    CursorMoved(usize),
    OpenStyleDialog,
    CloseStyleDialog,
    StyleApplyPreset(ThemePreset),
    StylePresetChoiceChanged,
    StyleSkinChanged,
    StyleFontChanged,
    StyleFontSizeChanged,
    StylePickBackground,
    StylePickTokenColor(TokenCategory),
    StyleWeightChanged(TokenCategory),
}

/// Преобразует начертание шрифта в слот FLTK `Courier..CourierBoldItalic`,
/// к которому `theme::bind_code_font_family` привязывает выбранный современный шрифт (Zed / Cascadia / Consolas).
pub fn resolve_fltk_font(_family: FontFamily, weight: FontSlantWeight) -> Font {
    match weight {
        FontSlantWeight::Regular => Font::Courier,
        FontSlantWeight::Bold => Font::CourierBold,
        FontSlantWeight::Italic => Font::CourierItalic,
        FontSlantWeight::BoldItalic => Font::CourierBoldItalic,
    }
}

/// Формирует таблицу стилей FLTK (`'A'..'F'`) на основе текущей конфигурации `EditorStyleConfig`.
pub fn build_fltk_style_table(cfg: &EditorStyleConfig) -> Vec<StyleTableEntry> {
    TokenCategory::ALL
        .iter()
        .map(|&cat| {
            let rule = cfg.rule(cat);
            let (r, g, b) = rule.rgb;
            StyleTableEntry {
                color: Color::from_rgb(r, g, b),
                font: resolve_fltk_font(cfg.font_family, rule.weight),
                size: cfg.font_size,
            }
        })
        .collect()
}

/// Визуальный компонент одной вкладки редактора внутри контейнера `Tabs`.
#[derive(Clone)]
pub struct EditorTab {
    /// ID документа в `Workspace`, к которому привязана эта вкладка
    pub doc_id: usize,
    /// Корневой контейнер вкладки (прямой потомок `Tabs`)
    pub container: Flex,
    /// Многострочное поле редактирования текста
    pub editor: TextEditor,
    /// Основной текстовый буфер FLTK (Gap Buffer)
    pub buffer: TextBuffer,
    /// Буфер стилей подсветки синтаксиса (`HighlightBuf` по п. 2 ЛР №5)
    pub style_buffer: TextBuffer,
}

impl EditorTab {
    /// Создаёт новую вкладку редактора со связкой `TextBuffer` + `HighlightBuf` и нумерацией строк.
    pub fn new(
        doc_id: usize,
        tab_title: &str,
        initial_text: &str,
        style_cfg: &EditorStyleConfig,
        area: (i32, i32, i32, i32),
        sender: app::Sender<AppMsg>,
    ) -> Self {
        let (x, y, w, h) = area;

        // Верхний отступ 6px нужен под соединительную «ножку» активной вкладки Fl_Tabs
        let mut container = Flex::new(x, y, w, h, None).with_type(FlexType::Row);
        container.set_label(&format!("  {}  ", tab_title));
        container.set_margins(0, 6, 0, 0);
        container.set_pad(0);

        let mut buffer = TextBuffer::default();
        buffer.set_tab_distance(4);

        let mut style_buffer = TextBuffer::default();

        if !initial_text.is_empty() {
            buffer.set_text(initial_text);
            // Небольшие файлы размечаем сразу без мерцания, а крупные (> 64 КБ) разберёт фоновый поток лексера
            if initial_text.len() <= 64 * 1024 {
                let initial_styles = highlight_rust_to_style_string(initial_text);
                style_buffer.set_text(&initial_styles);
            } else {
                style_buffer.set_text(&"A".repeat(initial_text.len()));
            }
        }

        let mut editor = TextEditor::default();
        editor.set_frame(FrameType::FlatBox);
        editor.set_cursor_style(fltk::text::Cursor::Simple);
        editor.wrap_mode(WrapMode::None, 0);

        // Вертикальная панель нумерации строк слева (тем же моноширинным шрифтом, как в Zed)
        editor.set_linenumber_width(54);
        editor.set_linenumber_font(Font::Courier);
        editor.set_linenumber_align(Align::Right);

        editor.set_buffer(buffer.clone());

        container.end();
        container.layout();

        // Подписываемся на любые изменения текста в буфере (ввод, удаление, вставка).
        // За O(1) синхронно выравниваем длину `style_buffer` с `buffer` (чтобы старые цвета не съезжали
        // ни на один байт), а полный перерасчёт синтаксиса отправляем в фоновый поток лексера.
        let sender_mod = sender;
        let mut style_sync = style_buffer.clone();
        buffer.add_modify_callback(move |pos, inserted, deleted, _restyled, _deleted_text| {
            if inserted > 0 || deleted > 0 {
                if deleted > 0 {
                    style_sync.remove(pos, pos + deleted);
                }
                if inserted > 0 {
                    let filler = "A".repeat(inserted as usize);
                    style_sync.insert(pos, &filler);
                }
                sender_mod.send(AppMsg::BufferChanged(doc_id));
            }
        });

        // Отслеживаем перемещение курсора стрелками и кликами мыши
        editor.handle(move |_ed, ev| match ev {
            Event::KeyUp | Event::Released => {
                sender.send(AppMsg::CursorMoved(doc_id));
                false
            }
            _ => false,
        });

        let mut tab = Self {
            doc_id,
            container,
            editor,
            buffer,
            style_buffer,
        };
        tab.apply_style_config(style_cfg);
        tab.adjust_gutter_width();
        tab
    }

    /// Применяет конфигурацию шрифтов, цветов фона, ярлычка вкладки и таблицы `StyleTableEntry`.
    pub fn apply_style_config(&mut self, cfg: &EditorStyleConfig) {
        let (bg_r, bg_g, bg_b) = cfg.background_rgb;
        let (gbg_r, gbg_g, gbg_b) = cfg.gutter_bg_rgb;
        let (gfg_r, gfg_g, gfg_b) = cfg.gutter_fg_rgb;
        let chrome = cfg.chrome_palette();
        let plain = cfg.rule(TokenCategory::Plain);

        // Оформление контейнера вкладки (в Fl_Tabs отвечает за вид НЕАКТИВНОЙ вкладки)
        self.container.set_color(Color::from_rgb(bg_r, bg_g, bg_b));
        self.container.set_selection_color(Color::from_rgb(
            chrome.inactive_tab_bg.0,
            chrome.inactive_tab_bg.1,
            chrome.inactive_tab_bg.2,
        ));
        self.container.set_label_color(Color::from_rgb(
            chrome.muted_fg.0,
            chrome.muted_fg.1,
            chrome.muted_fg.2,
        ));

        self.editor.set_color(Color::from_rgb(bg_r, bg_g, bg_b));
        self.editor
            .set_text_font(resolve_fltk_font(cfg.font_family, plain.weight));
        self.editor.set_text_size(cfg.font_size);
        self.editor
            .set_text_color(Color::from_rgb(plain.rgb.0, plain.rgb.1, plain.rgb.2));

        if chrome.is_dark {
            self.editor.set_cursor_color(Color::from_rgb(103, 232, 249));
            self.editor.set_selection_color(Color::from_rgb(
                chrome.accent_rgb.0,
                chrome.accent_rgb.1,
                chrome.accent_rgb.2,
            ));
        } else {
            self.editor.set_cursor_color(Color::from_rgb(37, 99, 235));
            self.editor.set_selection_color(Color::from_rgb(
                chrome.accent_rgb.0,
                chrome.accent_rgb.1,
                chrome.accent_rgb.2,
            ));
        }

        self.editor
            .set_linenumber_bgcolor(Color::from_rgb(gbg_r, gbg_g, gbg_b));
        self.editor
            .set_linenumber_fgcolor(Color::from_rgb(gfg_r, gfg_g, gfg_b));
        self.editor.set_linenumber_font(Font::Courier);
        self.editor.set_linenumber_size((cfg.font_size - 2).max(10));

        let entries = build_fltk_style_table(cfg);
        self.editor
            .set_highlight_data(self.style_buffer.clone(), entries);
        self.editor.redisplay_range(0, self.buffer.length());
        self.editor.redraw();
    }

    /// Обновляет буфер стилей `HighlightBuf` готовой строкой разметки и перерисовывает текст.
    pub fn apply_highlight_string(&mut self, style_str: &str) {
        self.style_buffer.set_text(style_str);
        self.editor.redisplay_range(0, self.buffer.length());
    }

    /// Обновляет текст на ярлычке вкладки (например, добавляет или убирает `*`).
    pub fn update_tab_label(&mut self, tab_title: &str) {
        self.container.set_label(&format!("  {}  ", tab_title));
    }

    /// Автоматически расширяет полосу нумерации строк при росте объёма текста (1000+, 10000+ строк).
    pub fn adjust_gutter_width(&mut self) {
        let total_lines = (self.buffer.count_lines(0, self.buffer.length()) + 1).max(1);
        let digits = total_lines.to_string().len().max(3) as i32;
        let target_width = 24 + digits * 9;
        if self.editor.linenumber_width() != target_width {
            self.editor.set_linenumber_width(target_width);
            self.editor.redraw();
        }
    }

    /// Возвращает текущие 1-базовые координаты курсора: (строка, столбец).
    pub fn cursor_line_col(&self) -> (usize, usize) {
        let pos = self.editor.insert_position().max(0);
        let line = (self.buffer.count_lines(0, pos) + 1) as usize;
        let line_start = self.editor.line_start(pos);
        let col = self
            .buffer
            .text_range(line_start, pos)
            .map(|s| s.chars().count() + 1)
            .unwrap_or(1);
        (line, col)
    }
}
