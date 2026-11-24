use fltk::{
    app,
    button::Button,
    dialog::{self, FileDialog, FileDialogAction, FileDialogOptions, FileDialogType},
    enums::{Align, Color, Event, FrameType, Shortcut},
    frame::Frame,
    group::{Flex, FlexType, Tabs, TabsOverflow},
    menu::{MenuFlag, SysMenuBar},
    prelude::*,
    window::Window,
};
use ideeditor::{
    model::document::Workspace,
    ui::editor_tab::{AppMsg, EditorTab},
};
use std::path::PathBuf;

const TAB_HEADER_HEIGHT: i32 = 28;
const FILE_DIALOG_FILTER: &str =
    "Текстовые и исходные файлы\t*.{rs,txt,md,toml,json,c,cpp,h,py}\nВсе файлы\t*";

/// Результат проверки несохранённых изменений перед закрытием документа.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum CloseDecision {
    Proceed,
    Cancel,
}

fn main() {
    let app = app::App::default().with_scheme(app::Scheme::Gtk);
    let (s, r) = app::channel::<AppMsg>();

    // Бизнес-логика (Domain Model) хранит данные всех открытых вкладок отдельно от GUI
    let mut workspace = Workspace::new();
    let mut tabs_ui: Vec<EditorTab> = Vec::new();

    let mut wind = Window::default()
        .with_size(1020, 680)
        .with_label("Rust Text Editor");
    wind.make_resizable(true);

    let mut root = Flex::default_fill().with_type(FlexType::Column);
    root.set_pad(0);

    // 1. Главное меню (раздел «Файл» по п. 2 ЛР №4)
    let mut menubar = SysMenuBar::default();
    menubar.add_emit(
        "Файл/Новый\t",
        Shortcut::Ctrl | 'n',
        MenuFlag::Normal,
        s,
        AppMsg::FileNew,
    );
    menubar.add_emit(
        "Файл/Открыть...\t",
        Shortcut::Ctrl | 'o',
        MenuFlag::Normal,
        s,
        AppMsg::FileOpen,
    );
    menubar.add_emit(
        "Файл/Сохранить\t",
        Shortcut::Ctrl | 's',
        MenuFlag::Normal,
        s,
        AppMsg::FileSave,
    );
    menubar.add_emit(
        "Файл/Сохранить как...\t",
        Shortcut::Ctrl | Shortcut::Shift | 's',
        MenuFlag::MenuDivider,
        s,
        AppMsg::FileSaveAs,
    );
    menubar.add_emit(
        "Файл/Закрыть вкладку\t",
        Shortcut::Ctrl | 'w',
        MenuFlag::MenuDivider,
        s,
        AppMsg::CloseActiveTab,
    );
    menubar.add_emit(
        "Файл/Выход\t",
        Shortcut::Ctrl | 'q',
        MenuFlag::Normal,
        s,
        AppMsg::AppExit,
    );
    root.fixed(&menubar, 28);

    // 2. Панель инструментов для быстрых действий с вкладками и файлами
    let mut toolbar = Flex::default().with_type(FlexType::Row);
    toolbar.set_frame(FrameType::ThinUpBox);
    toolbar.set_margins(6, 4, 6, 4);
    toolbar.set_pad(6);

    let mut btn_new = Button::default().with_label("+ Новый");
    btn_new.emit(s, AppMsg::FileNew);
    toolbar.fixed(&btn_new, 85);

    let mut btn_open = Button::default().with_label("Открыть...");
    btn_open.emit(s, AppMsg::FileOpen);
    toolbar.fixed(&btn_open, 95);

    let mut btn_save = Button::default().with_label("Сохранить");
    btn_save.emit(s, AppMsg::FileSave);
    toolbar.fixed(&btn_save, 95);

    let mut btn_save_as = Button::default().with_label("Сохранить как...");
    btn_save_as.emit(s, AppMsg::FileSaveAs);
    toolbar.fixed(&btn_save_as, 130);

    let mut btn_close_tab = Button::default().with_label("× Закрыть вкладку");
    btn_close_tab.emit(s, AppMsg::CloseActiveTab);
    toolbar.fixed(&btn_close_tab, 135);

    let _spacer = Frame::default();
    toolbar.end();
    root.fixed(&toolbar, 34);

    // 3. Многовкладочный контейнер (п. 7 продвинутого уровня ЛР №4)
    let mut tabs = Tabs::default();
    tabs.set_frame(FrameType::FlatBox);
    tabs.set_color(Color::from_rgb(226, 232, 240));
    tabs.set_selection_color(Color::White);
    tabs.handle_overflow(TabsOverflow::Pulldown);
    tabs.end();

    tabs.emit(s, AppMsg::TabSwitched);

    tabs.resize_callback(|t, x, y, w, h| {
        for child in t.clone().into_iter() {
            if let Some(mut grp) = child.as_group() {
                grp.resize(x, y + TAB_HEADER_HEIGHT, w, (h - TAB_HEADER_HEIGHT).max(10));
            }
        }
    });

    // 4. Нижняя статусная строка (п. 2 ЛР №4: путь к файлу и кодировка UTF-8)
    let mut status_bar = Frame::default();
    status_bar.set_align(Align::Left | Align::Inside);
    status_bar.set_frame(FrameType::ThinUpBox);
    status_bar.set_color(Color::from_rgb(241, 245, 249));
    root.fixed(&status_bar, 26);

    root.end();
    wind.end();

    // Перехватываем закрытие главного окна (крестик [X] или Alt+F4),
    // чтобы: 1) клавиша Esc случайно не закрывала редактор;
    //        2) перед выходом проверялись несохранённые вкладки.
    wind.set_callback(move |_| {
        if app::event() == Event::Close {
            s.send(AppMsg::AppExit);
        }
    });

    wind.show();

    // Создаём первую пустую вкладку при старте программы
    spawn_empty_tab(&mut workspace, &mut tabs, &mut tabs_ui, s);
    refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);

    while app.wait() {
        if let Some(msg) = r.recv() {
            match msg {
                AppMsg::FileNew => {
                    spawn_empty_tab(&mut workspace, &mut tabs, &mut tabs_ui, s);
                    refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                }
                AppMsg::FileOpen => {
                    handle_file_open(&mut workspace, &mut tabs, &mut tabs_ui, s);
                    refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                }
                AppMsg::FileSave => {
                    if let Some(active_id) = workspace.active_doc().map(|d| d.id) {
                        let _ = save_doc_with_ui(
                            &mut workspace,
                            &mut tabs,
                            &mut tabs_ui,
                            active_id,
                            false,
                        );
                        refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                    }
                }
                AppMsg::FileSaveAs => {
                    if let Some(active_id) = workspace.active_doc().map(|d| d.id) {
                        let _ = save_doc_with_ui(
                            &mut workspace,
                            &mut tabs,
                            &mut tabs_ui,
                            active_id,
                            true,
                        );
                        refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                    }
                }
                AppMsg::TabSwitched => {
                    if let Some(selected_grp) = tabs.value() {
                        if let Some(tab) = tabs_ui
                            .iter_mut()
                            .find(|t| t.container.as_widget_ptr() == selected_grp.as_widget_ptr())
                        {
                            workspace.set_active(tab.doc_id);
                            if let Some(saved_pos) = workspace.get(tab.doc_id).map(|d| d.cursor_pos)
                            {
                                tab.editor.set_insert_position(saved_pos);
                            }
                            let _ = tab.editor.take_focus();
                        }
                    }
                    refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                }
                AppMsg::CursorMoved(doc_id) => {
                    if let Some(tab) = tabs_ui.iter().find(|t| t.doc_id == doc_id) {
                        let cursor_pos = tab.editor.insert_position();
                        if let Some(doc) = workspace.get_mut(doc_id) {
                            doc.cursor_pos = cursor_pos;
                        }
                    }
                    refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                }
                AppMsg::BufferChanged(doc_id) => {
                    if let Some(tab) = tabs_ui.iter_mut().find(|t| t.doc_id == doc_id) {
                        let current_text = tab.buffer.text();
                        let cursor_pos = tab.editor.insert_position();
                        tab.adjust_gutter_width();

                        if let Some(doc) = workspace.get_mut(doc_id) {
                            doc.update_text(current_text);
                            doc.cursor_pos = cursor_pos;
                            tab.update_tab_label(&doc.tab_title());
                        }
                    }
                    tabs.redraw();
                    refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                }
                AppMsg::CloseActiveTab => {
                    if let Some(active_id) = workspace.active_doc().map(|d| d.id) {
                        if confirm_close_doc(&mut workspace, &mut tabs, &mut tabs_ui, active_id)
                            == CloseDecision::Proceed
                        {
                            remove_tab_by_id(&mut workspace, &mut tabs, &mut tabs_ui, active_id);

                            // Если закрыли последнюю вкладку — оставляем чистый пустой лист
                            if tabs_ui.is_empty() {
                                spawn_empty_tab(&mut workspace, &mut tabs, &mut tabs_ui, s);
                            } else if let Some(new_active_id) = workspace.active_doc().map(|d| d.id)
                            {
                                activate_tab_ui(&mut tabs, &mut tabs_ui, new_active_id);
                            }
                            tabs.redraw();
                            refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);
                        }
                    }
                }
                AppMsg::AppExit => {
                    // Проверяем по очереди все открытые документы с несохранёнными изменениями
                    let all_ids: Vec<usize> = workspace.documents().iter().map(|d| d.id).collect();
                    let mut can_exit = true;

                    for doc_id in all_ids {
                        let is_dirty = workspace.get(doc_id).map(|d| d.is_dirty).unwrap_or(false);
                        if is_dirty {
                            workspace.set_active(doc_id);
                            activate_tab_ui(&mut tabs, &mut tabs_ui, doc_id);
                            refresh_chrome(&workspace, &tabs_ui, &mut wind, &mut status_bar);

                            if confirm_close_doc(&mut workspace, &mut tabs, &mut tabs_ui, doc_id)
                                == CloseDecision::Cancel
                            {
                                can_exit = false;
                                break;
                            }
                        }
                    }

                    if can_exit {
                        app.quit();
                    }
                }
            }
        }
    }
}

/// Показывает системный диалог открытия файла (`FileDialog`) и загружает выбранный файл во вкладку.
fn handle_file_open(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut Vec<EditorTab>,
    sender: app::Sender<AppMsg>,
) {
    let mut dlg = FileDialog::new(FileDialogType::BrowseFile);
    dlg.set_title("Открыть текстовый файл");
    dlg.set_filter(FILE_DIALOG_FILTER);

    let chosen_path = match dlg.try_show() {
        Ok(FileDialogAction::Success) => {
            let p = dlg.filename();
            if p.as_os_str().is_empty() {
                return;
            }
            p
        }
        Ok(FileDialogAction::Cancelled) => return,
        Err(err) => {
            dialog::alert_default(&format!("Ошибка открытия системного диалога:\n{}", err));
            return;
        }
    };

    // Если сейчас открыта ровно одна чистая стартовая вкладка «Без имени 1»,
    // запомним её ID, чтобы аккуратно заменить её открытым файлом.
    let replaceable_empty_id = if workspace.documents().len() == 1 {
        workspace
            .documents()
            .first()
            .filter(|d| d.is_pristine_empty())
            .map(|d| d.id)
    } else {
        None
    };

    match workspace.open_doc(&chosen_path) {
        Ok((doc_id, is_new_tab)) => {
            if is_new_tab {
                if let Some(empty_id) = replaceable_empty_id {
                    remove_tab_by_id(workspace, tabs, tabs_ui, empty_id);
                    workspace.set_active(doc_id);
                }

                if let Some((title, text)) = workspace
                    .get(doc_id)
                    .map(|d| (d.tab_title(), d.current_content.clone()))
                {
                    let area = tab_client_area(tabs);
                    tabs.begin();
                    let tab = EditorTab::new(doc_id, &title, &text, area, sender);
                    tabs.end();

                    let _ = tabs.set_value(&tab.container);
                    let _ = tab.editor.clone().take_focus();
                    tabs_ui.push(tab);
                    tabs.redraw();
                }
            } else {
                // Файл уже был открыт — просто переключаем фокус на существующую вкладку
                activate_tab_ui(tabs, tabs_ui, doc_id);
            }
        }
        Err(err_msg) => {
            // Безопасная обработка ошибки I/O без panic! (п. 4 ЛР №4)
            dialog::alert_default(&err_msg);
        }
    }
}

/// Сохраняет документ `doc_id` (при необходимости запрашивая путь через `FileDialog`).
/// Возвращает `true`, если файл успешно сохранён на диск, и `false`, если пользователь отменил или возникла ошибка.
fn save_doc_with_ui(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut [EditorTab],
    doc_id: usize,
    force_save_as: bool,
) -> bool {
    let existing_path = workspace.get(doc_id).and_then(|d| d.file_path.clone());

    let target_path: PathBuf = match existing_path {
        Some(path) if !force_save_as => path,
        _ => {
            let mut dlg = FileDialog::new(FileDialogType::BrowseSaveFile);
            dlg.set_title("Сохранить файл как...");
            dlg.set_filter(FILE_DIALOG_FILTER);
            dlg.set_option(FileDialogOptions::SaveAsConfirm | FileDialogOptions::NewFolder);

            if let Some(doc) = workspace.get(doc_id) {
                let suggested = doc
                    .file_path
                    .as_ref()
                    .and_then(|p| p.file_name())
                    .map(|n| n.to_string_lossy().into_owned())
                    .unwrap_or_else(|| format!("untitled_{}.txt", doc.untitled_num.max(1)));
                dlg.set_preset_file(&suggested);
            }

            match dlg.try_show() {
                Ok(FileDialogAction::Success) => {
                    let p = dlg.filename();
                    if p.as_os_str().is_empty() {
                        return false;
                    }
                    p
                }
                Ok(FileDialogAction::Cancelled) => return false,
                Err(err) => {
                    dialog::alert_default(&format!("Ошибка вызова диалога сохранения:\n{}", err));
                    return false;
                }
            }
        }
    };

    let save_result = match workspace.get_mut(doc_id) {
        Some(doc) => doc.save_as(&target_path),
        None => return false,
    };

    match save_result {
        Ok(()) => {
            if let Some(new_label) = workspace.get(doc_id).map(|d| d.tab_title()) {
                if let Some(tab) = tabs_ui.iter_mut().find(|t| t.doc_id == doc_id) {
                    tab.update_tab_label(&new_label);
                }
            }
            tabs.redraw();
            true
        }
        Err(err_msg) => {
            dialog::alert_default(&err_msg);
            false
        }
    }
}

/// Если в документе есть несохранённые изменения, выводит окно предупреждения
/// «Сохранить / Не сохранять / Отмена» (п. 5 ЛР №4).
fn confirm_close_doc(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut [EditorTab],
    doc_id: usize,
) -> CloseDecision {
    let (is_dirty, name) = match workspace.get(doc_id) {
        Some(d) => (d.is_dirty, d.display_name()),
        None => return CloseDecision::Proceed,
    };

    if !is_dirty {
        return CloseDecision::Proceed;
    }

    let prompt = format!(
        "В документе «{}» есть несохранённые изменения.\nСохранить файл перед закрытием?",
        name
    );

    match dialog::choice2_default(&prompt, "Сохранить", "Не сохранять", "Отмена")
    {
        Some(0) => {
            if save_doc_with_ui(workspace, tabs, tabs_ui, doc_id, false) {
                CloseDecision::Proceed
            } else {
                CloseDecision::Cancel
            }
        }
        Some(1) => CloseDecision::Proceed,
        _ => CloseDecision::Cancel,
    }
}

/// Удаляет вкладку из `Tabs`, `tabs_ui` и `Workspace`.
fn remove_tab_by_id(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut Vec<EditorTab>,
    doc_id: usize,
) {
    if let Some(pos) = tabs_ui.iter().position(|t| t.doc_id == doc_id) {
        let removed = tabs_ui.remove(pos);
        tabs.remove(&removed.container);
    }
    workspace.close_doc(doc_id);
}

/// Переключает визуальный контейнер `Tabs` и фокус ввода на вкладку `doc_id`.
fn activate_tab_ui(tabs: &mut Tabs, tabs_ui: &mut [EditorTab], doc_id: usize) {
    if let Some(tab) = tabs_ui.iter_mut().find(|t| t.doc_id == doc_id) {
        let _ = tabs.set_value(&tab.container);
        let _ = tab.editor.take_focus();
    }
}

/// Вычисляет рабочую область под ярлычками `Tabs`.
fn tab_client_area(tabs: &Tabs) -> (i32, i32, i32, i32) {
    (
        tabs.x(),
        tabs.y() + TAB_HEADER_HEIGHT,
        tabs.w(),
        (tabs.h() - TAB_HEADER_HEIGHT).max(10),
    )
}

/// Добавляет новую пустую вкладку и в `Workspace`, и в визуальный контейнер `Tabs`.
fn spawn_empty_tab(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut Vec<EditorTab>,
    sender: app::Sender<AppMsg>,
) {
    let doc_id = workspace.create_empty_doc();
    let title = workspace
        .get(doc_id)
        .map(|d| d.tab_title())
        .unwrap_or_else(|| "Без имени 1".to_string());

    let area = tab_client_area(tabs);

    tabs.begin();
    let tab = EditorTab::new(doc_id, &title, "", area, sender);
    tabs.end();

    let _ = tabs.set_value(&tab.container);
    let _ = tab.editor.clone().take_focus();
    tabs_ui.push(tab);
    tabs.redraw();
}

/// Синхронизирует заголовок главного окна (`*`) и нижнюю статусную строку с активной вкладкой.
fn refresh_chrome(
    workspace: &Workspace,
    tabs_ui: &[EditorTab],
    wind: &mut Window,
    status_bar: &mut Frame,
) {
    if let Some(doc) = workspace.active_doc() {
        wind.set_label(&doc.window_title());

        let (line, col) = tabs_ui
            .iter()
            .find(|t| t.doc_id == doc.id)
            .map(|t| t.cursor_line_col())
            .unwrap_or((1, 1));

        status_bar.set_label(&format!(
            "{}   |   Стр: {}, Стб: {}",
            doc.status_bar_text(),
            line,
            col
        ));
    }
}
