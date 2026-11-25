use std::fs;
use std::path::{Path, PathBuf};

/// Состояние одного открытого документа (одной вкладки редактора).
/// Не зависит от библиотеки FLTK — чистая бизнес-логика (Domain Model).
#[derive(Debug, Clone)]
pub struct Document {
    /// Уникальный внутренний идентификатор вкладки в рамках сессии (никогда не повторяется)
    pub id: usize,
    /// Номер для отображения «Без имени N» (освобождается при закрытии вкладки)
    pub untitled_num: usize,
    /// Путь к файлу на диске (None, если файл только создан и ещё ни разу не сохранялся)
    pub file_path: Option<PathBuf>,
    /// Последний сохранённый на диске текст (нужен для честного расчёта флага изменений)
    saved_content: String,
    /// Текущий текст в редакторе
    pub current_content: String,
    /// Позиция курсора (индекс байта в UTF-8 буфере)
    pub cursor_pos: i32,
    /// Есть ли несохранённые изменения (для отображения символа `*`)
    pub is_dirty: bool,
    /// Монотонно растущий номер ревизии текста (для фильтрации устаревших ответов фонового лексера)
    pub revision: u64,
    /// Кодировка документа
    pub encoding: &'static str,
}

impl Document {
    /// Создаёт новый пустой несохранённый документ с указанным номером `untitled_num`.
    pub fn new_empty(id: usize, untitled_num: usize) -> Self {
        Self {
            id,
            untitled_num,
            file_path: None,
            saved_content: String::new(),
            current_content: String::new(),
            cursor_pos: 0,
            is_dirty: false,
            revision: 0,
            encoding: "UTF-8",
        }
    }

    /// Безопасно открывает текстовый файл с диска в кодировке UTF-8.
    /// Вместо аварийного завершения (`panic!`) возвращает `Result`:
    /// `Ok(Document)` при успехе или `Err(описание ошибки)` при сбое.
    pub fn open_from_path(id: usize, path: impl AsRef<Path>) -> Result<Self, String> {
        let path_buf = path.as_ref().to_path_buf();
        let content = fs::read_to_string(&path_buf).map_err(|err| {
            format!(
                "Не удалось прочитать файл «{}»:\n{}",
                path_buf.display(),
                err
            )
        })?;

        Ok(Self {
            id,
            untitled_num: 0,
            file_path: Some(path_buf),
            saved_content: content.clone(),
            current_content: content,
            cursor_pos: 0,
            is_dirty: false,
            revision: 0,
            encoding: "UTF-8",
        })
    }

    /// Проверяет, является ли вкладка совершенно чистой стартовой заглушкой
    /// (не привязана к файлу, пуста и не редактировалась).
    pub fn is_pristine_empty(&self) -> bool {
        self.file_path.is_none() && !self.is_dirty && self.current_content.is_empty()
    }

    /// Обновляет текст документа, увеличивает номер ревизии и пересчитывает флаг `is_dirty`.
    pub fn update_text(&mut self, new_text: String) {
        self.revision = self.revision.wrapping_add(1);
        self.is_dirty = new_text != self.saved_content;
        self.current_content = new_text;
    }

    /// Сохраняет документ по его текущему пути.
    /// Если путь ещё не задан (`None`), сообщает, что нужно вызвать «Сохранить как...».
    pub fn save(&mut self) -> Result<(), String> {
        match &self.file_path {
            Some(path) => {
                let target = path.clone();
                self.save_as(target)
            }
            None => Err("Файл ещё не имеет имени на диске. Используйте «Сохранить как...».".into()),
        }
    }

    /// Сохраняет документ по указанному пути и сбрасывает флаг несохранённых изменений `*`.
    pub fn save_as(&mut self, path: impl AsRef<Path>) -> Result<(), String> {
        let path_buf = path.as_ref().to_path_buf();
        fs::write(&path_buf, &self.current_content).map_err(|err| {
            format!(
                "Не удалось сохранить файл «{}»:\n{}",
                path_buf.display(),
                err
            )
        })?;

        self.file_path = Some(path_buf);
        self.saved_content = self.current_content.clone();
        self.is_dirty = false;
        Ok(())
    }

    /// Человекочитаемое имя документа без звёздочки (например: `main.rs` или `Без имени 1`).
    pub fn display_name(&self) -> String {
        self.file_path
            .as_ref()
            .and_then(|p| p.file_name())
            .map(|name| name.to_string_lossy().into_owned())
            .unwrap_or_else(|| format!("Без имени {}", self.untitled_num.max(1)))
    }

    /// Короткое имя документа для заголовка вкладки (например: `main.rs *` или `Без имени 1`).
    pub fn tab_title(&self) -> String {
        let base_name = self.display_name();
        if self.is_dirty {
            format!("{} *", base_name)
        } else {
            base_name
        }
    }

    /// Полный заголовок главного окна приложения (с символом `*` при наличии изменений).
    pub fn window_title(&self) -> String {
        let path_part = self
            .file_path
            .as_ref()
            .map(|p| p.display().to_string())
            .unwrap_or_else(|| self.display_name());

        let dirty_mark = if self.is_dirty { "*" } else { "" };
        format!("{}{} — Rust Text Editor", dirty_mark, path_part)
    }

    /// Текст для нижней статусной строки: путь к файлу, кодировка и число строк.
    pub fn status_bar_text(&self) -> String {
        let path_str = self
            .file_path
            .as_ref()
            .map(|p| p.display().to_string())
            .unwrap_or_else(|| "[Новый несохранённый файл]".to_string());

        let line_count = self.line_count();
        format!(
            " Файл: {}   |   Строк: {}   |   Кодировка: {}",
            path_str, line_count, self.encoding
        )
    }

    /// Подсчитывает количество строк в текущем тексте (минимум 1 строка даже для пустого файла).
    pub fn line_count(&self) -> usize {
        if self.current_content.is_empty() {
            1
        } else {
            self.current_content.split('\n').count()
        }
    }
}

/// Менеджер всех открытых документов (модель многовкладочного редактора).
#[derive(Debug, Default)]
pub struct Workspace {
    documents: Vec<Document>,
    active_doc_id: Option<usize>,
    next_id: usize,
}

impl Workspace {
    pub fn new() -> Self {
        Self {
            documents: Vec::new(),
            active_doc_id: None,
            next_id: 1,
        }
    }

    /// Находит наименьший свободный номер 1, 2, 3... среди открытых вкладок «Без имени».
    fn next_free_untitled_num(&self) -> usize {
        (1..)
            .find(|candidate| {
                !self
                    .documents
                    .iter()
                    .any(|d| d.file_path.is_none() && d.untitled_num == *candidate)
            })
            .unwrap_or(1)
    }

    /// Создаёт новую пустую вкладку и делает её активной.
    pub fn create_empty_doc(&mut self) -> usize {
        let id = self.next_id;
        self.next_id += 1;
        let untitled_num = self.next_free_untitled_num();
        self.documents.push(Document::new_empty(id, untitled_num));
        self.active_doc_id = Some(id);
        id
    }

    /// Открывает файл с диска (или переключается на уже открытую вкладку с тем же путём).
    /// Возвращает `(id_документа, была_ли_создана_новая_вкладка)`.
    pub fn open_doc(&mut self, path: impl AsRef<Path>) -> Result<(usize, bool), String> {
        let target_path = path.as_ref();

        // Если этот файл уже открыт в одной из вкладок — просто активируем её
        if let Some(existing) = self
            .documents
            .iter()
            .find(|d| d.file_path.as_deref() == Some(target_path))
        {
            let id = existing.id;
            self.active_doc_id = Some(id);
            return Ok((id, false));
        }

        let id = self.next_id;
        let doc = Document::open_from_path(id, target_path)?;
        self.next_id += 1;
        self.documents.push(doc);
        self.active_doc_id = Some(id);
        Ok((id, true))
    }

    /// Возвращает ссылку для чтения на документ по `id`.
    pub fn get(&self, id: usize) -> Option<&Document> {
        self.documents.iter().find(|d| d.id == id)
    }

    /// Возвращает изменяемую ссылку на документ по `id`.
    pub fn get_mut(&mut self, id: usize) -> Option<&mut Document> {
        self.documents.iter_mut().find(|d| d.id == id)
    }

    /// Возвращает ссылку на текущий активный документ.
    pub fn active_doc(&self) -> Option<&Document> {
        let id = self.active_doc_id?;
        self.get(id)
    }

    /// Возвращает изменяемую ссылку на текущий активный документ.
    pub fn active_doc_mut(&mut self) -> Option<&mut Document> {
        let id = self.active_doc_id?;
        self.get_mut(id)
    }

    /// Устанавливает активный документ по `id`.
    pub fn set_active(&mut self, id: usize) {
        if self.documents.iter().any(|d| d.id == id) {
            self.active_doc_id = Some(id);
        }
    }

    /// Закрывает документ по `id` и выбирает соседнюю вкладку активной.
    pub fn close_doc(&mut self, id: usize) {
        if let Some(pos) = self.documents.iter().position(|d| d.id == id) {
            self.documents.remove(pos);
            if self.active_doc_id == Some(id) {
                let fallback_idx = pos.saturating_sub(1);
                self.active_doc_id = self.documents.get(fallback_idx).map(|d| d.id);
            }
        }
    }

    /// Список всех открытых документов.
    pub fn documents(&self) -> &[Document] {
        &self.documents
    }

    /// Проверяет, есть ли хотя бы одна вкладка с несохранёнными изменениями.
    pub fn has_unsaved_changes(&self) -> bool {
        self.documents.iter().any(|d| d.is_dirty)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn dirty_flag_tracks_modifications_accurately() {
        let mut doc = Document::new_empty(1, 1);
        assert!(!doc.is_dirty);
        assert_eq!(doc.tab_title(), "Без имени 1");

        doc.update_text("fn main() {}".to_string());
        assert!(doc.is_dirty);
        assert_eq!(doc.tab_title(), "Без имени 1 *");
        assert!(doc.window_title().starts_with('*'));

        // Если вернули исходный текст — звёздочка пропадает
        doc.update_text(String::new());
        assert!(!doc.is_dirty);
    }

    #[test]
    fn untitled_numbers_are_reused_when_tabs_close() {
        let mut ws = Workspace::new();
        let id1 = ws.create_empty_doc();
        let id2 = ws.create_empty_doc();
        assert_eq!(
            ws.get(id1).map(|d| d.tab_title()).as_deref(),
            Some("Без имени 1")
        );
        assert_eq!(
            ws.get(id2).map(|d| d.tab_title()).as_deref(),
            Some("Без имени 2")
        );

        ws.close_doc(id1);
        let id3 = ws.create_empty_doc();
        assert_ne!(id1, id3); // внутренний ID уникален
        assert_eq!(
            ws.get(id3).map(|d| d.tab_title()).as_deref(),
            Some("Без имени 1")
        );
    }

    #[test]
    fn save_and_open_roundtrip_works_without_panic() {
        let temp_path = std::env::temp_dir().join("ideeditor_step1_test.txt");
        let mut doc = Document::new_empty(1, 1);
        doc.update_text("Привет, Rust!\nВторая строка".to_string());
        assert_eq!(doc.line_count(), 2);

        doc.save_as(&temp_path).expect("Файл должен сохраниться");
        assert!(!doc.is_dirty);

        let loaded = Document::open_from_path(2, &temp_path).expect("Файл должен открыться");
        assert_eq!(loaded.current_content, "Привет, Rust!\nВторая строка");
        assert!(!loaded.is_dirty);

        let _ = fs::remove_file(&temp_path);
    }

    #[test]
    fn opening_nonexistent_file_returns_err_instead_of_panic() {
        let result = Document::open_from_path(1, "nonexistent_dir_12345/missing.rs");
        assert!(result.is_err());
    }
}
