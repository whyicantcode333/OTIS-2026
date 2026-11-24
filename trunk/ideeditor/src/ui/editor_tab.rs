use fltk::{
    app,
    enums::{Align, Color, Event, Font, FrameType},
    group::{Flex, FlexType},
    prelude::*,
    text::{TextBuffer, TextEditor, WrapMode},
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
    CursorMoved(usize),
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
    /// Текстовый буфер FLTK (внутри реализован как структура данных Gap Buffer)
    pub buffer: TextBuffer,
}

impl EditorTab {
    /// Создаёт новую вкладку редактора с синхронизированной панелью нумерации строк.
    pub fn new(
        doc_id: usize,
        tab_title: &str,
        initial_text: &str,
        area: (i32, i32, i32, i32),
        sender: app::Sender<AppMsg>,
    ) -> Self {
        let (x, y, w, h) = area;

        // Контейнер самой вкладки (заголовок с отступами отображается на ярлычке Tabs)
        let mut container = Flex::new(x, y, w, h, None).with_type(FlexType::Row);
        container.set_label(&format!("  {}  ", tab_title));
        container.set_margins(0, 6, 0, 0);
        container.set_pad(0);

        let mut buffer = TextBuffer::default();
        buffer.set_tab_distance(4);
        if !initial_text.is_empty() {
            buffer.set_text(initial_text);
        }

        let mut editor = TextEditor::default();
        editor.set_frame(FrameType::FlatBox);
        editor.set_color(Color::White);
        editor.set_text_font(Font::Courier);
        editor.set_text_size(15);
        editor.set_text_color(Color::from_rgb(30, 41, 59));
        editor.set_cursor_style(fltk::text::Cursor::Simple);
        editor.set_cursor_color(Color::from_rgb(37, 99, 235));
        editor.set_selection_color(Color::from_rgb(191, 219, 254));
        editor.wrap_mode(WrapMode::None, 0);

        // --- Требование п. 3 ЛР №4: вертикальная панель нумерации строк слева ---
        // Встроенный рендер FLTK жёстко синхронизирован со скроллингом и высотой строк.
        editor.set_linenumber_width(52);
        editor.set_linenumber_font(Font::Helvetica);
        editor.set_linenumber_size(13);
        editor.set_linenumber_fgcolor(Color::from_rgb(71, 85, 105));
        editor.set_linenumber_bgcolor(Color::from_rgb(241, 245, 249));
        editor.set_linenumber_align(Align::Right);

        editor.set_buffer(buffer.clone());

        container.end();
        container.layout();

        // Подписываемся на любые изменения текста в буфере (ввод, удаление, вставка)
        let sender_mod = sender;
        buffer.add_modify_callback(move |_pos, inserted, deleted, _restyled, _deleted_text| {
            if inserted > 0 || deleted > 0 {
                sender_mod.send(AppMsg::BufferChanged(doc_id));
            }
        });

        // Отслеживаем перемещение курсора стрелками и кликами мыши для каждой вкладки
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
        };
        tab.adjust_gutter_width();
        tab
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
