use crate::model::lexer::TokenCategory;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::Path;

/// Имя файла конфигурации стилей по умолчанию (создаётся рядом с проектом/исполняемым файлом).
pub const DEFAULT_CONFIG_FILENAME: &str = "editor_styles.json";

/// Визуальный стиль оформления элементов интерфейса (кнопок, выпадающих списков, шапки).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum UiSkin {
    /// Скруглённые стеклянные капсулы с преломлением и бликом Френеля
    #[default]
    LiquidGlass,
    /// Базовый строгий плоский стиль современных IDE (Zed / VS Code)
    BasicFlat,
}

impl Serialize for UiSkin {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        match self {
            UiSkin::LiquidGlass => serializer.serialize_str("LiquidGlass"),
            UiSkin::BasicFlat => serializer.serialize_str("BasicFlat"),
        }
    }
}

impl<'de> Deserialize<'de> for UiSkin {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = String::deserialize(deserializer)?;
        Ok(match raw.as_str() {
            "BasicFlat" | "FlatMinimal" => UiSkin::BasicFlat,
            _ => UiSkin::LiquidGlass,
        })
    }
}

impl UiSkin {
    pub const ALL: [UiSkin; 2] = [UiSkin::LiquidGlass, UiSkin::BasicFlat];

    pub fn display_name(self) -> &'static str {
        match self {
            UiSkin::LiquidGlass => "Liquid Glass (Стеклянные капсулы)",
            UiSkin::BasicFlat => "Базовый минималистичный (IDE)",
        }
    }

    pub fn to_index(self) -> i32 {
        match self {
            UiSkin::LiquidGlass => 0,
            UiSkin::BasicFlat => 1,
        }
    }

    pub fn from_index(idx: i32) -> Self {
        match idx {
            1 => UiSkin::BasicFlat,
            _ => UiSkin::LiquidGlass,
        }
    }
}

/// Набор готовых популярных цветовых тем редактора.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ThemePreset {
    LiquidGlassDark,
    LiquidGlassLight,
    OneDark,
    Dracula,
    TokyoNight,
    CatppuccinMocha,
    GitHubLight,
    SolarizedLight,
}

impl ThemePreset {
    pub const ALL: [ThemePreset; 8] = [
        ThemePreset::LiquidGlassDark,
        ThemePreset::LiquidGlassLight,
        ThemePreset::OneDark,
        ThemePreset::Dracula,
        ThemePreset::TokyoNight,
        ThemePreset::CatppuccinMocha,
        ThemePreset::GitHubLight,
        ThemePreset::SolarizedLight,
    ];

    pub fn display_name(self) -> &'static str {
        match self {
            ThemePreset::LiquidGlassDark => "Liquid Glass Midnight (Тёмное стекло)",
            ThemePreset::LiquidGlassLight => "Liquid Glass Ice (Светлое стекло)",
            ThemePreset::OneDark => "One Dark Pro (Zed - Atom)",
            ThemePreset::Dracula => "Dracula (Неоновый пурпур)",
            ThemePreset::TokyoNight => "Tokyo Night (Ночной индиго)",
            ThemePreset::CatppuccinMocha => "Catppuccin Mocha (Тёплая пастель)",
            ThemePreset::GitHubLight => "GitHub Light (Чистая светлая)",
            ThemePreset::SolarizedLight => "Solarized Light (Тёплая кремовая)",
        }
    }

    pub fn from_index(idx: i32) -> Option<Self> {
        Self::ALL.get(idx as usize).copied()
    }
}

/// Базовое семейство моноширинных шрифтов кода (чистый Rust-тип без привязки к FLTK).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum FontFamily {
    /// Современный шрифт в стиле Zed (Zed Mono -> JetBrains Mono -> Cascadia Mono -> Consolas)
    #[default]
    ZedCascadia,
    /// Разработчицкий моноширинный шрифт (JetBrains Mono -> Fira Code -> Cascadia Code)
    JetBrainsFira,
    /// Классический чёткий шрифт Windows IDE (Consolas)
    Consolas,
    /// Строгий печатный моноширинный шрифт (Courier New)
    CourierClassic,
}

impl Serialize for FontFamily {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let tag = match self {
            FontFamily::ZedCascadia => "ZedCascadia",
            FontFamily::JetBrainsFira => "JetBrainsFira",
            FontFamily::Consolas => "Consolas",
            FontFamily::CourierClassic => "CourierClassic",
        };
        serializer.serialize_str(tag)
    }
}

impl<'de> Deserialize<'de> for FontFamily {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let raw = String::deserialize(deserializer)?;
        Ok(match raw.as_str() {
            // Автоматически переводим старое значение "Courier" на современный Zed / Cascadia Mono
            "ZedCascadia" | "Courier" => FontFamily::ZedCascadia,
            "JetBrainsFira" | "Screen" => FontFamily::JetBrainsFira,
            "Consolas" | "Helvetica" => FontFamily::Consolas,
            "CourierClassic" | "Times" => FontFamily::CourierClassic,
            _ => FontFamily::ZedCascadia,
        })
    }
}

impl FontFamily {
    pub const ALL: [FontFamily; 4] = [
        FontFamily::ZedCascadia,
        FontFamily::JetBrainsFira,
        FontFamily::Consolas,
        FontFamily::CourierClassic,
    ];

    pub fn display_name(self) -> &'static str {
        match self {
            FontFamily::ZedCascadia => "Zed - Cascadia Mono",
            FontFamily::JetBrainsFira => "JetBrains - Fira Mono",
            FontFamily::Consolas => "Consolas",
            FontFamily::CourierClassic => "Courier New",
        }
    }

    pub fn to_index(self) -> i32 {
        match self {
            FontFamily::ZedCascadia => 0,
            FontFamily::JetBrainsFira => 1,
            FontFamily::Consolas => 2,
            FontFamily::CourierClassic => 3,
        }
    }

    pub fn from_index(idx: i32) -> Self {
        match idx {
            1 => FontFamily::JetBrainsFira,
            2 => FontFamily::Consolas,
            3 => FontFamily::CourierClassic,
            _ => FontFamily::ZedCascadia,
        }
    }
}

/// Начертание шрифта для конкретной категории токенов (п. 3 ЛР №5: regular, bold, italic).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum FontSlantWeight {
    Regular,
    Bold,
    Italic,
    BoldItalic,
}

impl FontSlantWeight {
    pub const ALL: [FontSlantWeight; 4] = [
        FontSlantWeight::Regular,
        FontSlantWeight::Bold,
        FontSlantWeight::Italic,
        FontSlantWeight::BoldItalic,
    ];

    pub fn display_name(self) -> &'static str {
        match self {
            FontSlantWeight::Regular => "Regular (Обычный)",
            FontSlantWeight::Bold => "Bold (Полужирный)",
            FontSlantWeight::Italic => "Italic (Курсив)",
            FontSlantWeight::BoldItalic => "Bold Italic (Жирный курсив)",
        }
    }

    pub fn to_index(self) -> i32 {
        match self {
            FontSlantWeight::Regular => 0,
            FontSlantWeight::Bold => 1,
            FontSlantWeight::Italic => 2,
            FontSlantWeight::BoldItalic => 3,
        }
    }

    pub fn from_index(idx: i32) -> Self {
        match idx {
            1 => FontSlantWeight::Bold,
            2 => FontSlantWeight::Italic,
            3 => FontSlantWeight::BoldItalic,
            _ => FontSlantWeight::Regular,
        }
    }
}

/// Настройка цвета (RGB) и начертания для одной категории синтаксических токенов.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct TokenStyleRule {
    pub rgb: (u8, u8, u8),
    pub weight: FontSlantWeight,
}

/// Вычисляемая палитра всех оболочечных элементов приложения (меню, тулбар, вкладки, статусбар, диалоги).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct UiChromePalette {
    pub is_dark: bool,
    /// Фон окон, меню, панели инструментов и статусной строки
    pub panel_bg: (u8, u8, u8),
    /// Фон кнопок, выпадающих списков и полей ввода
    pub surface_bg: (u8, u8, u8),
    /// Фон полосы вкладок позади ярлычков
    pub tab_strip_bg: (u8, u8, u8),
    /// Фон неактивных ярлычков вкладок
    pub inactive_tab_bg: (u8, u8, u8),
    /// Основной контрастный цвет текста интерфейса (меню, кнопки, активная вкладка)
    pub text_fg: (u8, u8, u8),
    /// Приглушённый цвет текста (неактивные вкладки, подсказки)
    pub muted_fg: (u8, u8, u8),
    /// Акцентный цвет выделения пунктов меню и ползунков
    pub accent_rgb: (u8, u8, u8),
}

/// Полная конфигурация внешнего вида, стиля интерфейса и подсветки синтаксиса редактора.
/// Сериализуется в JSON на диск и восстанавливается при старте программы (п. 4 ЛР №5).
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct EditorStyleConfig {
    /// Стиль отрисовки виджетов интерфейса (Liquid Glass, GTK, Gleam, Aqua, Flat)
    #[serde(default)]
    pub ui_skin: UiSkin,
    /// Семейство шрифта текста кода
    pub font_family: FontFamily,
    /// Размер шрифта в пунктах (10..=28)
    pub font_size: i32,
    /// Цвет фона текстового поля (RGB)
    pub background_rgb: (u8, u8, u8),
    /// Цвет фона панели нумерации строк (RGB)
    pub gutter_bg_rgb: (u8, u8, u8),
    /// Цвет цифр на панели нумерации строк (RGB)
    pub gutter_fg_rgb: (u8, u8, u8),
    /// Стили для 6 категорий токенов (Plain, Keyword, TypeOrMacro, StringLiteral, NumberLiteral, Comment)
    pub token_rules: [TokenStyleRule; 6],
}

impl Default for EditorStyleConfig {
    fn default() -> Self {
        Self::from_preset(ThemePreset::LiquidGlassLight)
    }
}

impl EditorStyleConfig {
    /// Создаёт конфигурацию по выбранному пресету цветовой темы.
    pub fn from_preset(preset: ThemePreset) -> Self {
        match preset {
            ThemePreset::LiquidGlassDark => Self {
                ui_skin: UiSkin::LiquidGlass,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (16, 24, 41),
                gutter_bg_rgb: (22, 33, 56),
                gutter_fg_rgb: (125, 156, 196),
                token_rules: [
                    TokenStyleRule {
                        rgb: (226, 238, 255),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (192, 132, 252),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (103, 232, 249),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (110, 231, 183),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (251, 191, 36),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (116, 144, 184),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::LiquidGlassLight => Self {
                ui_skin: UiSkin::LiquidGlass,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (245, 250, 255),
                gutter_bg_rgb: (228, 239, 252),
                gutter_fg_rgb: (71, 98, 136),
                token_rules: [
                    TokenStyleRule {
                        rgb: (20, 36, 62),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (109, 40, 217),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (2, 119, 189),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (13, 128, 80),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (194, 65, 12),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (100, 122, 153),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::OneDark => Self {
                ui_skin: UiSkin::BasicFlat,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (30, 34, 42),
                gutter_bg_rgb: (40, 44, 52),
                gutter_fg_rgb: (130, 142, 162),
                token_rules: [
                    TokenStyleRule {
                        rgb: (220, 223, 228),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (198, 120, 221),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (86, 182, 194),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (152, 195, 121),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (209, 154, 102),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (127, 132, 142),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::Dracula => Self {
                ui_skin: UiSkin::LiquidGlass,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (40, 42, 54),
                gutter_bg_rgb: (50, 53, 68),
                gutter_fg_rgb: (139, 148, 184),
                token_rules: [
                    TokenStyleRule {
                        rgb: (248, 248, 242),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (255, 121, 198),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (139, 233, 253),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (241, 250, 140),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (189, 147, 249),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (98, 114, 164),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::TokyoNight => Self {
                ui_skin: UiSkin::BasicFlat,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (26, 27, 38),
                gutter_bg_rgb: (32, 34, 48),
                gutter_fg_rgb: (115, 122, 162),
                token_rules: [
                    TokenStyleRule {
                        rgb: (192, 202, 245),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (187, 154, 247),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (122, 162, 247),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (158, 206, 106),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (255, 158, 100),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (102, 110, 148),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::CatppuccinMocha => Self {
                ui_skin: UiSkin::LiquidGlass,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (30, 30, 46),
                gutter_bg_rgb: (38, 39, 58),
                gutter_fg_rgb: (147, 153, 178),
                token_rules: [
                    TokenStyleRule {
                        rgb: (205, 214, 244),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (203, 166, 247),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (137, 180, 250),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (166, 227, 161),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (250, 179, 135),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (127, 132, 156),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::GitHubLight => Self {
                ui_skin: UiSkin::BasicFlat,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (255, 255, 255),
                gutter_bg_rgb: (241, 245, 249),
                gutter_fg_rgb: (71, 85, 105),
                token_rules: [
                    TokenStyleRule {
                        rgb: (30, 41, 59),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (124, 58, 237),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (3, 105, 161),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (21, 128, 61),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (194, 65, 12),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (100, 116, 139),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
            ThemePreset::SolarizedLight => Self {
                ui_skin: UiSkin::BasicFlat,
                font_family: FontFamily::ZedCascadia,
                font_size: 15,
                background_rgb: (253, 246, 227),
                gutter_bg_rgb: (238, 232, 213),
                gutter_fg_rgb: (101, 123, 131),
                token_rules: [
                    TokenStyleRule {
                        rgb: (88, 110, 117),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (133, 153, 0),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (38, 139, 210),
                        weight: FontSlantWeight::Bold,
                    },
                    TokenStyleRule {
                        rgb: (42, 161, 152),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (211, 54, 130),
                        weight: FontSlantWeight::Regular,
                    },
                    TokenStyleRule {
                        rgb: (147, 161, 161),
                        weight: FontSlantWeight::Italic,
                    },
                ],
            },
        }
    }

    /// Короткие алиасы светлой и тёмной палитр для совместимости и тестов.
    pub fn light_preset() -> Self {
        Self::from_preset(ThemePreset::LiquidGlassLight)
    }

    pub fn dark_preset() -> Self {
        Self::from_preset(ThemePreset::LiquidGlassDark)
    }

    /// Определяет, является ли текущий фон редактора тёмным.
    pub fn is_dark(&self) -> bool {
        let (r, g, b) = self.background_rgb;
        (r as u32 * 299 + g as u32 * 587 + b as u32 * 114) / 1000 < 135
    }

    /// Вычисляет гармоничную палитру для всего окна приложения, меню, тулбара, вкладок и диалогов.
    pub fn chrome_palette(&self) -> UiChromePalette {
        let (r, g, b) = self.background_rgb;
        let is_dark = self.is_dark();
        let kw_rgb = self.rule(TokenCategory::TypeOrMacro).rgb;

        if is_dark {
            let panel_bg = (
                r.saturating_add(10),
                g.saturating_add(12),
                b.saturating_add(18),
            );
            let surface_bg = (
                r.saturating_add(24),
                g.saturating_add(28),
                b.saturating_add(38),
            );
            let tab_strip_bg = (
                r.saturating_sub(4).max(10),
                g.saturating_sub(4).max(12),
                b.saturating_sub(4).max(18),
            );
            let inactive_tab_bg = (
                r.saturating_add(14),
                g.saturating_add(16),
                b.saturating_add(24),
            );
            let accent_rgb = (
                (kw_rgb.0 / 2).saturating_add(30),
                (kw_rgb.1 / 2).saturating_add(40),
                (kw_rgb.2 / 2).saturating_add(65),
            );
            UiChromePalette {
                is_dark: true,
                panel_bg,
                surface_bg,
                tab_strip_bg,
                inactive_tab_bg,
                text_fg: (232, 238, 248),
                muted_fg: (158, 172, 196),
                accent_rgb,
            }
        } else {
            let panel_bg = (
                r.saturating_sub(14),
                g.saturating_sub(10),
                b.saturating_sub(5),
            );
            let surface_bg = (
                r.saturating_sub(6),
                g.saturating_sub(3),
                b.saturating_sub(1),
            );
            let tab_strip_bg = (
                r.saturating_sub(26),
                g.saturating_sub(20),
                b.saturating_sub(10),
            );
            let inactive_tab_bg = (
                r.saturating_sub(18),
                g.saturating_sub(13),
                b.saturating_sub(6),
            );
            UiChromePalette {
                is_dark: false,
                panel_bg,
                surface_bg,
                tab_strip_bg,
                inactive_tab_bg,
                text_fg: (20, 32, 52),
                muted_fg: (82, 98, 122),
                accent_rgb: (186, 218, 255),
            }
        }
    }

    /// Если текущие цвета совпадают с одним из готовых пресетов — возвращает его индекс.
    pub fn matching_preset_index(&self) -> Option<i32> {
        for (idx, preset) in ThemePreset::ALL.iter().enumerate() {
            let sample = Self::from_preset(*preset);
            if sample.background_rgb == self.background_rgb
                && sample.token_rules == self.token_rules
            {
                return Some(idx as i32);
            }
        }
        None
    }

    /// Возвращает правило стиля для конкретной категории токена.
    pub fn rule(&self, category: TokenCategory) -> TokenStyleRule {
        self.token_rules[category as usize]
    }

    /// Изменяет правило стиля для конкретной категории токена.
    pub fn set_rule(&mut self, category: TokenCategory, rule: TokenStyleRule) {
        self.token_rules[category as usize] = rule;
    }

    /// Сохраняет конфигурацию стилей в JSON-файл на диске.
    pub fn save_to_file(&self, path: impl AsRef<Path>) -> Result<(), String> {
        let json = serde_json::to_string_pretty(self)
            .map_err(|e| format!("Ошибка сериализации настроек стилей: {}", e))?;
        fs::write(path.as_ref(), json).map_err(|e| {
            format!(
                "Не удалось записать файл конфигурации «{}»:\n{}",
                path.as_ref().display(),
                e
            )
        })
    }

    /// Считывает конфигурацию стилей из JSON-файла (при отсутствии файла возвращает стандартную).
    pub fn load_or_default(path: impl AsRef<Path>) -> Self {
        let path_ref = path.as_ref();
        if !path_ref.exists() {
            return Self::default();
        }
        match fs::read_to_string(path_ref) {
            Ok(raw) => serde_json::from_str(&raw).unwrap_or_default(),
            Err(_) => Self::default(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn style_config_serializes_and_deserializes_cleanly() {
        let temp_path = std::env::temp_dir().join("ideeditor_style_test.json");
        let mut cfg = EditorStyleConfig::from_preset(ThemePreset::Dracula);
        cfg.ui_skin = UiSkin::LiquidGlass;
        cfg.font_size = 18;
        cfg.set_rule(
            TokenCategory::Keyword,
            TokenStyleRule {
                rgb: (255, 80, 120),
                weight: FontSlantWeight::BoldItalic,
            },
        );

        cfg.save_to_file(&temp_path)
            .expect("Конфиг должен сохраниться");
        let loaded = EditorStyleConfig::load_or_default(&temp_path);
        assert_eq!(loaded, cfg);

        let _ = fs::remove_file(&temp_path);
    }

    #[test]
    fn all_presets_produce_consistent_dark_or_light_chrome() {
        for preset in ThemePreset::ALL {
            let cfg = EditorStyleConfig::from_preset(preset);
            let chrome = cfg.chrome_palette();
            assert_eq!(chrome.is_dark, cfg.is_dark());
        }
    }

    #[test]
    fn legacy_ui_skin_names_deserialize_gracefully() {
        let flat: UiSkin = serde_json::from_str("\"FlatMinimal\"").unwrap();
        let legacy: UiSkin = serde_json::from_str("\"GleamGlossy\"").unwrap();
        assert_eq!(flat, UiSkin::BasicFlat);
        assert_eq!(legacy, UiSkin::LiquidGlass);
    }
}
