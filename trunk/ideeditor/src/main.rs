use fltk::{
    app,
    dialog::{self, FileDialog, FileDialogAction, FileDialogOptions, FileDialogType},
    enums::{Align, Color, Event, FrameType, Shortcut},
    frame::Frame,
    group::{Flex, FlexType, Tabs, TabsOverflow},
    menu::{MenuBar, MenuFlag},
    prelude::*,
    window::Window,
};
use ideeditor::{
    model::{
        async_highlighter::{BackgroundHighlighter, HighlightRequest},
        document::Workspace,
        style_config::{DEFAULT_CONFIG_FILENAME, EditorStyleConfig, ThemePreset, TokenStyleRule},
    },
    ui::{
        color_picker::{confirm_unsaved_dialog, pick_color_dialog},
        editor_tab::{AppMsg, EditorTab},
        style_dialog::StyleDialog,
        theme::{
            apply_global_fltk_theme, configure_frameless_os_window, draw_frameless_window_border,
            draw_glass_bar_background, register_custom_frames,
        },
        titlebar::{CustomTitleBar, TITLEBAR_HEIGHT, create_resize_grip},
    },
};
use std::{cell::RefCell, path::PathBuf, rc::Rc};

const TAB_HEADER_HEIGHT: i32 = 28;
const FILE_DIALOG_FILTER: &str =
    "Исходные файлы Rust и текст\t*.{rs,txt,md,toml,json,c,cpp,h,py}\nВсе файлы\t*";

/// Результат проверки несохранённых изменений перед закрытием документа.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum CloseDecision {
    Proceed,
    Cancel,
}

/// Все виджеты оболочки главного окна, которые синхронно перекрашиваются при смене темы/скина.
struct MainChromeWidgets<'a> {
    wind: &'a mut Window,
    titlebar: &'a mut CustomTitleBar,
    menubar: &'a mut MenuBar,
    tabs: &'a mut Tabs,
    status_row: &'a mut Flex,
    status_bar: &'a mut Frame,
    grip_color: &'a Rc<RefCell<(u8, u8, u8)>>,
    live_cfg: &'a Rc<RefCell<EditorStyleConfig>>,
}

fn main() {
    let app = app::App::default();
    register_custom_frames();
    let (s, r) = app::channel::<AppMsg>();

    // Запускаем фоновый рабочий поток лексического анализатора (std::thread + std::sync::mpsc)
    let highlighter = BackgroundHighlighter::new(move || {
        s.send(AppMsg::HighlightReady);
    });

    // Загружаем сохранённую конфигурацию стилей из editor_styles.json (по умолчанию Liquid Glass Ice)
    let mut style_config = EditorStyleConfig::load_or_default(DEFAULT_CONFIG_FILENAME);
    apply_global_fltk_theme(&style_config);
    let live_cfg = Rc::new(RefCell::new(style_config.clone()));

    let mut workspace = Workspace::new();
    let mut tabs_ui: Vec<EditorTab> = Vec::new();

    let mut wind = Window::default()
        .with_size(1080, 700)
        .with_label("Rust Text Editor")
        .center_screen();
    // Убираем системную белую рамку Windows — используем собственную шапку с 3 точками macOS/Linux
    wind.set_border(false);
    wind.make_resizable(true);

    let mut outer = Flex::default_fill().with_type(FlexType::Column);
    outer.set_margins(1, 1, 1, 1);
    outer.set_pad(0);

    // 1. Единая безрамочная шапка: 3 цветные точки (Закрыть / Свернуть / Развернуть) + компактное меню + заголовок
    let mut menubar_holder: Option<MenuBar> = None;
    let mut titlebar = CustomTitleBar::new(
        &wind,
        "Rust Text Editor",
        &style_config,
        true,
        move || {
            s.send(AppMsg::AppExit);
        },
        |bar| {
            let mut mb = MenuBar::default();
            mb.set_frame(FrameType::NoBox);
            mb.set_text_size(14);
            mb.add_emit(
                "Файл/Новый\t",
                Shortcut::Ctrl | 'n',
                MenuFlag::Normal,
                s,
                AppMsg::FileNew,
            );
            mb.add_emit(
                "Файл/Открыть...\t",
                Shortcut::Ctrl | 'o',
                MenuFlag::Normal,
                s,
                AppMsg::FileOpen,
            );
            mb.add_emit(
                "Файл/Сохранить\t",
                Shortcut::Ctrl | 's',
                MenuFlag::Normal,
                s,
                AppMsg::FileSave,
            );
            mb.add_emit(
                "Файл/Сохранить как...\t",
                Shortcut::Ctrl | Shortcut::Shift | 's',
                MenuFlag::MenuDivider,
                s,
                AppMsg::FileSaveAs,
            );
            mb.add_emit(
                "Файл/Закрыть вкладку\t",
                Shortcut::Ctrl | 'w',
                MenuFlag::MenuDivider,
                s,
                AppMsg::CloseActiveTab,
            );
            mb.add_emit(
                "Файл/Выход\t",
                Shortcut::Ctrl | 'q',
                MenuFlag::Normal,
                s,
                AppMsg::AppExit,
            );
            mb.add_emit(
                "Вид/Настройки стилей и шрифтов...\t",
                Shortcut::Ctrl | Shortcut::Shift | 't',
                MenuFlag::MenuDivider,
                s,
                AppMsg::OpenStyleDialog,
            );
            for preset in ThemePreset::ALL {
                let menu_path = format!("Вид/Тема: {}\t", preset.display_name());
                mb.add_emit(
                    &menu_path,
                    Shortcut::None,
                    MenuFlag::Normal,
                    s,
                    AppMsg::StyleApplyPreset(preset),
                );
            }
            bar.fixed(&mb, 118);
            menubar_holder = Some(mb);
        },
    );
    let mut menubar = menubar_holder.expect("MenuBar создан внутри шапки");
    outer.fixed(&titlebar.bar, TITLEBAR_HEIGHT);

    // 2. Многовкладочный контейнер кода (без лишней дублирующей панели кнопок!)
    let mut tabs = Tabs::default();
    tabs.set_frame(FrameType::FlatBox);
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

    // 3. Нижняя стеклянная статусная строка + уголок растягивания окна мышью
    let mut status_row = Flex::default().with_type(FlexType::Row);
    status_row.set_frame(FrameType::NoBox);
    status_row.set_margins(8, 0, 2, 0);
    status_row.set_pad(4);

    let mut status_bar = Frame::default();
    status_bar.set_frame(FrameType::NoBox);
    status_bar.set_align(Align::Left | Align::Inside);
    status_bar.set_label_size(13);

    let (resize_grip, grip_color) = create_resize_grip(&wind, &style_config);
    status_row.fixed(&resize_grip, 20);

    status_row.end();
    outer.fixed(&status_row, 26);

    outer.end();
    wind.end();

    {
        let cfg_status = Rc::clone(&live_cfg);
        status_row.draw(move |sr| {
            let cfg = cfg_status.borrow();
            let chrome = cfg.chrome_palette();
            draw_glass_bar_background(
                sr.x(),
                sr.y(),
                sr.w(),
                sr.h(),
                chrome.panel_bg,
                chrome.is_dark,
                cfg.ui_skin,
                false,
            );
            for idx in 0..sr.children() {
                if let Some(mut child) = sr.child(idx) {
                    sr.draw_child(&mut child);
                }
            }
        });
    }

    {
        let cfg_win = Rc::clone(&live_cfg);
        wind.draw(move |w| {
            w.draw_children();
            draw_frameless_window_border(w.w(), w.h(), &cfg_win.borrow());
        });
    }

    // Создаём отдельное безрамочное окно настройки стилей и шрифтов
    let mut style_dialog = StyleDialog::new(&style_config, s);

    apply_main_chrome_theme(
        &style_config,
        MainChromeWidgets {
            wind: &mut wind,
            titlebar: &mut titlebar,
            menubar: &mut menubar,
            tabs: &mut tabs,
            status_row: &mut status_row,
            status_bar: &mut status_bar,
            grip_color: &grip_color,
            live_cfg: &live_cfg,
        },
    );

    wind.set_callback(move |_| {
        if app::event() == Event::Close {
            s.send(AppMsg::AppExit);
        }
    });

    wind.show();
    configure_frameless_os_window(&wind, true);

    // Создаём первую пустую вкладку при старте программы
    spawn_empty_tab(&mut workspace, &mut tabs, &mut tabs_ui, &style_config, s);
    refresh_chrome(
        &workspace,
        &tabs_ui,
        &mut wind,
        &mut titlebar,
        &mut status_row,
        &mut status_bar,
    );

    while app.wait() {
        if let Some(msg) = r.recv() {
            match msg {
                AppMsg::FileNew => {
                    spawn_empty_tab(&mut workspace, &mut tabs, &mut tabs_ui, &style_config, s);
                    refresh_chrome(
                        &workspace,
                        &tabs_ui,
                        &mut wind,
                        &mut titlebar,
                        &mut status_row,
                        &mut status_bar,
                    );
                }
                AppMsg::FileOpen => {
                    handle_file_open(
                        &mut workspace,
                        &mut tabs,
                        &mut tabs_ui,
                        &style_config,
                        &highlighter,
                        s,
                    );
                    refresh_chrome(
                        &workspace,
                        &tabs_ui,
                        &mut wind,
                        &mut titlebar,
                        &mut status_row,
                        &mut status_bar,
                    );
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
                        refresh_chrome(
                            &workspace,
                            &tabs_ui,
                            &mut wind,
                            &mut titlebar,
                            &mut status_row,
                            &mut status_bar,
                        );
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
                        refresh_chrome(
                            &workspace,
                            &tabs_ui,
                            &mut wind,
                            &mut titlebar,
                            &mut status_row,
                            &mut status_bar,
                        );
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
                    refresh_chrome(
                        &workspace,
                        &tabs_ui,
                        &mut wind,
                        &mut titlebar,
                        &mut status_row,
                        &mut status_bar,
                    );
                }
                AppMsg::CursorMoved(doc_id) => {
                    if let Some(tab) = tabs_ui.iter().find(|t| t.doc_id == doc_id) {
                        let cursor_pos = tab.editor.insert_position();
                        if let Some(doc) = workspace.get_mut(doc_id) {
                            doc.cursor_pos = cursor_pos;
                        }
                    }
                    refresh_chrome(
                        &workspace,
                        &tabs_ui,
                        &mut wind,
                        &mut titlebar,
                        &mut status_row,
                        &mut status_bar,
                    );
                }
                AppMsg::BufferChanged(doc_id) => {
                    if let Some(tab) = tabs_ui.iter_mut().find(|t| t.doc_id == doc_id) {
                        let current_text = tab.buffer.text();
                        let cursor_pos = tab.editor.insert_position();
                        tab.adjust_gutter_width();

                        if let Some(doc) = workspace.get_mut(doc_id) {
                            doc.update_text(current_text.clone());
                            doc.cursor_pos = cursor_pos;
                            tab.update_tab_label(&doc.tab_title());

                            // Неблокирующе отправляем текст в фоновый поток лексера через mpsc-канал
                            highlighter.submit(HighlightRequest {
                                doc_id,
                                revision: doc.revision,
                                text: current_text,
                            });
                        }
                    }
                    tabs.redraw();
                    refresh_chrome(
                        &workspace,
                        &tabs_ui,
                        &mut wind,
                        &mut titlebar,
                        &mut status_row,
                        &mut status_bar,
                    );
                }
                AppMsg::HighlightReady => {
                    // Забираем все готовые ответы из mpsc-канала фонового лексера
                    while let Some(resp) = highlighter.try_recv() {
                        let is_current_rev = workspace
                            .get(resp.doc_id)
                            .map(|doc| doc.revision == resp.revision)
                            .unwrap_or(false);

                        if is_current_rev
                            && let Some(tab) = tabs_ui.iter_mut().find(|t| t.doc_id == resp.doc_id)
                            && tab.buffer.length() as usize == resp.style_string.len()
                        {
                            tab.apply_highlight_string(&resp.style_string);
                        }
                    }
                }
                AppMsg::OpenStyleDialog => {
                    style_dialog.show(&style_config);
                }
                AppMsg::CloseStyleDialog => {
                    style_dialog.hide();
                }
                AppMsg::StyleApplyPreset(preset) => {
                    let prev_skin = style_config.ui_skin;
                    let prev_family = style_config.font_family;
                    let prev_size = style_config.font_size;
                    style_config = EditorStyleConfig::from_preset(preset);
                    style_config.ui_skin = prev_skin;
                    style_config.font_family = prev_family;
                    style_config.font_size = prev_size;
                    apply_and_persist_styles(
                        &style_config,
                        MainChromeWidgets {
                            wind: &mut wind,
                            titlebar: &mut titlebar,
                            menubar: &mut menubar,
                            tabs: &mut tabs,
                            status_row: &mut status_row,
                            status_bar: &mut status_bar,
                            grip_color: &grip_color,
                            live_cfg: &live_cfg,
                        },
                        &mut tabs_ui,
                        &mut style_dialog,
                    );
                }
                AppMsg::StylePresetChoiceChanged => {
                    if let Some(preset) = style_dialog.selected_preset() {
                        let prev_skin = style_config.ui_skin;
                        let prev_family = style_config.font_family;
                        let prev_size = style_config.font_size;
                        style_config = EditorStyleConfig::from_preset(preset);
                        style_config.ui_skin = prev_skin;
                        style_config.font_family = prev_family;
                        style_config.font_size = prev_size;
                        apply_and_persist_styles(
                            &style_config,
                            MainChromeWidgets {
                                wind: &mut wind,
                                titlebar: &mut titlebar,
                                menubar: &mut menubar,
                                tabs: &mut tabs,
                                status_row: &mut status_row,
                                status_bar: &mut status_bar,
                                grip_color: &grip_color,
                                live_cfg: &live_cfg,
                            },
                            &mut tabs_ui,
                            &mut style_dialog,
                        );
                    }
                }
                AppMsg::StyleSkinChanged => {
                    style_config.ui_skin = style_dialog.selected_ui_skin();
                    apply_and_persist_styles(
                        &style_config,
                        MainChromeWidgets {
                            wind: &mut wind,
                            titlebar: &mut titlebar,
                            menubar: &mut menubar,
                            tabs: &mut tabs,
                            status_row: &mut status_row,
                            status_bar: &mut status_bar,
                            grip_color: &grip_color,
                            live_cfg: &live_cfg,
                        },
                        &mut tabs_ui,
                        &mut style_dialog,
                    );
                }
                AppMsg::StyleFontChanged => {
                    style_config.font_family = style_dialog.selected_font_family();
                    apply_and_persist_styles(
                        &style_config,
                        MainChromeWidgets {
                            wind: &mut wind,
                            titlebar: &mut titlebar,
                            menubar: &mut menubar,
                            tabs: &mut tabs,
                            status_row: &mut status_row,
                            status_bar: &mut status_bar,
                            grip_color: &grip_color,
                            live_cfg: &live_cfg,
                        },
                        &mut tabs_ui,
                        &mut style_dialog,
                    );
                }
                AppMsg::StyleFontSizeChanged => {
                    style_config.font_size = style_dialog.selected_font_size();
                    apply_and_persist_styles(
                        &style_config,
                        MainChromeWidgets {
                            wind: &mut wind,
                            titlebar: &mut titlebar,
                            menubar: &mut menubar,
                            tabs: &mut tabs,
                            status_row: &mut status_row,
                            status_bar: &mut status_bar,
                            grip_color: &grip_color,
                            live_cfg: &live_cfg,
                        },
                        &mut tabs_ui,
                        &mut style_dialog,
                    );
                }
                AppMsg::StylePickBackground => {
                    if let Some(chosen) = pick_color_dialog(
                        "Выбор цвета фона редактора",
                        style_config.background_rgb,
                        &style_config,
                    ) {
                        style_config.background_rgb = chosen;
                        if style_config.is_dark() {
                            style_config.gutter_bg_rgb = (
                                chosen.0.saturating_add(10),
                                chosen.1.saturating_add(12),
                                chosen.2.saturating_add(16),
                            );
                            style_config.gutter_fg_rgb = (140, 156, 184);
                        } else {
                            style_config.gutter_bg_rgb = (
                                chosen.0.saturating_sub(14),
                                chosen.1.saturating_sub(10),
                                chosen.2.saturating_sub(6),
                            );
                            style_config.gutter_fg_rgb = (71, 85, 105);
                        }
                        apply_and_persist_styles(
                            &style_config,
                            MainChromeWidgets {
                                wind: &mut wind,
                                titlebar: &mut titlebar,
                                menubar: &mut menubar,
                                tabs: &mut tabs,
                                status_row: &mut status_row,
                                status_bar: &mut status_bar,
                                grip_color: &grip_color,
                                live_cfg: &live_cfg,
                            },
                            &mut tabs_ui,
                            &mut style_dialog,
                        );
                    }
                }
                AppMsg::StylePickTokenColor(category) => {
                    let old_rule = style_config.rule(category);
                    let title = format!("Палитра цвета — {}", category.display_name());
                    if let Some(chosen) = pick_color_dialog(&title, old_rule.rgb, &style_config) {
                        style_config.set_rule(
                            category,
                            TokenStyleRule {
                                rgb: chosen,
                                weight: old_rule.weight,
                            },
                        );
                        apply_and_persist_styles(
                            &style_config,
                            MainChromeWidgets {
                                wind: &mut wind,
                                titlebar: &mut titlebar,
                                menubar: &mut menubar,
                                tabs: &mut tabs,
                                status_row: &mut status_row,
                                status_bar: &mut status_bar,
                                grip_color: &grip_color,
                                live_cfg: &live_cfg,
                            },
                            &mut tabs_ui,
                            &mut style_dialog,
                        );
                    }
                }
                AppMsg::StyleWeightChanged(category) => {
                    if let Some(new_weight) = style_dialog.selected_weight(category) {
                        let old_rule = style_config.rule(category);
                        style_config.set_rule(
                            category,
                            TokenStyleRule {
                                rgb: old_rule.rgb,
                                weight: new_weight,
                            },
                        );
                        apply_and_persist_styles(
                            &style_config,
                            MainChromeWidgets {
                                wind: &mut wind,
                                titlebar: &mut titlebar,
                                menubar: &mut menubar,
                                tabs: &mut tabs,
                                status_row: &mut status_row,
                                status_bar: &mut status_bar,
                                grip_color: &grip_color,
                                live_cfg: &live_cfg,
                            },
                            &mut tabs_ui,
                            &mut style_dialog,
                        );
                    }
                }
                AppMsg::CloseActiveTab => {
                    if let Some(active_id) = workspace.active_doc().map(|d| d.id) {
                        if confirm_close_doc(
                            &mut workspace,
                            &mut tabs,
                            &mut tabs_ui,
                            &style_config,
                            active_id,
                        ) == CloseDecision::Proceed
                        {
                            remove_tab_by_id(&mut workspace, &mut tabs, &mut tabs_ui, active_id);

                            if tabs_ui.is_empty() {
                                spawn_empty_tab(
                                    &mut workspace,
                                    &mut tabs,
                                    &mut tabs_ui,
                                    &style_config,
                                    s,
                                );
                            } else if let Some(new_active_id) = workspace.active_doc().map(|d| d.id)
                            {
                                activate_tab_ui(&mut tabs, &mut tabs_ui, new_active_id);
                            }
                            tabs.redraw();
                            refresh_chrome(
                                &workspace,
                                &tabs_ui,
                                &mut wind,
                                &mut titlebar,
                                &mut status_row,
                                &mut status_bar,
                            );
                        }
                    }
                }
                AppMsg::AppExit => {
                    let all_ids: Vec<usize> = workspace.documents().iter().map(|d| d.id).collect();
                    let mut can_exit = true;

                    for doc_id in all_ids {
                        let is_dirty = workspace.get(doc_id).map(|d| d.is_dirty).unwrap_or(false);
                        if is_dirty {
                            workspace.set_active(doc_id);
                            activate_tab_ui(&mut tabs, &mut tabs_ui, doc_id);
                            refresh_chrome(
                                &workspace,
                                &tabs_ui,
                                &mut wind,
                                &mut titlebar,
                                &mut status_row,
                                &mut status_bar,
                            );

                            if confirm_close_doc(
                                &mut workspace,
                                &mut tabs,
                                &mut tabs_ui,
                                &style_config,
                                doc_id,
                            ) == CloseDecision::Cancel
                            {
                                can_exit = false;
                                break;
                            }
                        }
                    }

                    if can_exit {
                        style_dialog.hide();
                        app.quit();
                    }
                }
            }
        }
    }
}

/// Окрашивает все элементы оболочки главного безрамочного окна.
fn apply_main_chrome_theme(cfg: &EditorStyleConfig, chrome_ui: MainChromeWidgets<'_>) {
    *chrome_ui.live_cfg.borrow_mut() = cfg.clone();

    let chrome = cfg.chrome_palette();
    let panel_col = Color::from_rgb(chrome.panel_bg.0, chrome.panel_bg.1, chrome.panel_bg.2);
    let text_col = Color::from_rgb(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);
    let accent_col = Color::from_rgb(
        chrome.accent_rgb.0,
        chrome.accent_rgb.1,
        chrome.accent_rgb.2,
    );
    let strip_col = Color::from_rgb(
        chrome.tab_strip_bg.0,
        chrome.tab_strip_bg.1,
        chrome.tab_strip_bg.2,
    );
    let editor_bg_col = Color::from_rgb(
        cfg.background_rgb.0,
        cfg.background_rgb.1,
        cfg.background_rgb.2,
    );

    chrome_ui.wind.set_color(strip_col);
    chrome_ui.titlebar.apply_theme(cfg);

    chrome_ui.menubar.set_color(panel_col);
    chrome_ui.menubar.set_text_color(text_col);
    chrome_ui.menubar.set_selection_color(accent_col);

    chrome_ui.tabs.set_color(strip_col);
    chrome_ui.tabs.set_selection_color(editor_bg_col);
    chrome_ui.tabs.set_label_color(text_col);

    chrome_ui.status_bar.set_label_color(text_col);
    *chrome_ui.grip_color.borrow_mut() = chrome.muted_fg;
    chrome_ui.status_row.redraw();
}

/// Мгновенно применяет новую тему/скин ко ВСЕМ окнам и вкладкам и сохраняет `editor_styles.json`.
fn apply_and_persist_styles(
    style_cfg: &EditorStyleConfig,
    chrome_ui: MainChromeWidgets<'_>,
    tabs_ui: &mut [EditorTab],
    style_dialog: &mut StyleDialog,
) {
    apply_global_fltk_theme(style_cfg);
    apply_main_chrome_theme(style_cfg, chrome_ui);

    for tab in tabs_ui.iter_mut() {
        tab.apply_style_config(style_cfg);
    }

    style_dialog.sync_from_config(style_cfg);
    app::redraw();

    if let Err(err_msg) = style_cfg.save_to_file(DEFAULT_CONFIG_FILENAME) {
        dialog::alert_default(&err_msg);
    }
}

/// Показывает системный диалог открытия файла (`FileDialog`) и загружает выбранный файл во вкладку.
fn handle_file_open(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut Vec<EditorTab>,
    style_cfg: &EditorStyleConfig,
    highlighter: &BackgroundHighlighter,
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

                if let Some((title, text, rev)) = workspace
                    .get(doc_id)
                    .map(|d| (d.tab_title(), d.current_content.clone(), d.revision))
                {
                    let is_large = text.len() > 64 * 1024;
                    let area = tab_client_area(tabs);
                    tabs.begin();
                    let tab = EditorTab::new(doc_id, &title, &text, style_cfg, area, sender);
                    tabs.end();

                    if is_large {
                        highlighter.submit(HighlightRequest {
                            doc_id,
                            revision: rev,
                            text,
                        });
                    }

                    let _ = tabs.set_value(&tab.container);
                    let _ = tab.editor.clone().take_focus();
                    tabs_ui.push(tab);
                    tabs.redraw();
                }
            } else {
                activate_tab_ui(tabs, tabs_ui, doc_id);
            }
        }
        Err(err_msg) => {
            dialog::alert_default(&err_msg);
        }
    }
}

/// Сохраняет документ `doc_id` (при необходимости запрашивая путь через `FileDialog`).
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
                    .unwrap_or_else(|| format!("untitled_{}.rs", doc.untitled_num.max(1)));
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

/// Если в документе есть несохранённые изменения, выводит безрамочное окно предупреждения.
fn confirm_close_doc(
    workspace: &mut Workspace,
    tabs: &mut Tabs,
    tabs_ui: &mut [EditorTab],
    style_cfg: &EditorStyleConfig,
    doc_id: usize,
) -> CloseDecision {
    let (is_dirty, name) = match workspace.get(doc_id) {
        Some(d) => (d.is_dirty, d.display_name()),
        None => return CloseDecision::Proceed,
    };

    if !is_dirty {
        return CloseDecision::Proceed;
    }

    match confirm_unsaved_dialog(&name, style_cfg) {
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
    style_cfg: &EditorStyleConfig,
    sender: app::Sender<AppMsg>,
) {
    let doc_id = workspace.create_empty_doc();
    let title = workspace
        .get(doc_id)
        .map(|d| d.tab_title())
        .unwrap_or_else(|| "Без имени 1".to_string());

    let area = tab_client_area(tabs);

    tabs.begin();
    let tab = EditorTab::new(doc_id, &title, "", style_cfg, area, sender);
    tabs.end();

    let _ = tabs.set_value(&tab.container);
    let _ = tab.editor.clone().take_focus();
    tabs_ui.push(tab);
    tabs.redraw();
}

/// Синхронизирует кастомную шапку окна (`*`) и нижнюю статусную строку с активной вкладкой.
fn refresh_chrome(
    workspace: &Workspace,
    tabs_ui: &[EditorTab],
    wind: &mut Window,
    titlebar: &mut CustomTitleBar,
    status_row: &mut Flex,
    status_bar: &mut Frame,
) {
    if let Some(doc) = workspace.active_doc() {
        let win_title = doc.window_title();
        wind.set_label(&win_title);
        titlebar.set_title(&win_title);

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
        status_row.redraw();
    }
}
