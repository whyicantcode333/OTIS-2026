use crate::model::style_config::{EditorStyleConfig, FontFamily, UiSkin};
use fltk::{
    app::{self, Scheme},
    button::Button,
    draw,
    enums::{Align, Color, ColorDepth, Event, Font, FrameType},
    image::{Image, RgbImage, RgbScaling},
    menu::Choice,
    prelude::*,
    valuator::HorValueSlider,
    window::Window,
};
use std::{
    cell::Cell,
    collections::{HashMap, HashSet},
    path::Path,
    rc::Rc,
    sync::OnceLock,
};

static INSTALLED_FAMILIES_LOWER: OnceLock<HashSet<String>> = OnceLock::new();

/// Пользовательские слоты FrameType для HiDPI SDF-виджетов (покой / наведение / нажатие).
pub const LIQUID_GLASS_UP_BOX: FrameType = FrameType::DiamondUpBox;
pub const LIQUID_GLASS_DOWN_BOX: FrameType = FrameType::DiamondDownBox;
pub const LIQUID_GLASS_HOVER_BOX: FrameType = FrameType::OShadowBox;

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
enum ControlState {
    Normal,
    Hovered,
    Pressed,
}

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
struct CapsuleCacheKey {
    skin: UiSkin,
    phys_w: i32,
    phys_h: i32,
    fg_rgb: (u8, u8, u8),
    bg_rgb: (u8, u8, u8),
    state: ControlState,
}

thread_local! {
    static CURRENT_UI_SKIN: Cell<UiSkin> = const { Cell::new(UiSkin::LiquidGlass) };
    static CURRENT_PANEL_BG: Cell<(u8, u8, u8)> = const { Cell::new((235, 242, 250)) };
    static LAST_BOUND_CODE_FAMILY: Cell<Option<FontFamily>> = const { Cell::new(None) };
    static GLASS_CACHE: std::cell::RefCell<HashMap<CapsuleCacheKey, RgbImage>> =
        std::cell::RefCell::new(HashMap::new());
}

/// Знаковое расстояние (SDF) до скруглённой капсулы.
#[inline]
fn sdf_rounded_rect(px: f32, py: f32, cx: f32, cy: f32, half_w: f32, half_h: f32, r: f32) -> f32 {
    let qx = (px - cx).abs() - (half_w - r);
    let qy = (py - cy).abs() - (half_h - r);
    let out_dist = (qx.max(0.0).powi(2) + qy.max(0.0).powi(2)).sqrt();
    let in_dist = qx.max(qy).min(0.0);
    out_dist + in_dist - r
}

#[inline]
fn mix_u8(a: u8, b: u8, t: f32) -> u8 {
    let t = t.clamp(0.0, 1.0);
    (a as f32 * (1.0 - t) + b as f32 * t)
        .round()
        .clamp(0.0, 255.0) as u8
}

/// Генерирует изображение капсулы «Liquid Glass» в физическом разрешении экрана (HiDPI 1:1)
/// с 4×4 суперсэмплингом краёв (16x MSAA), диагональным преломлением и бликом Френеля.
fn build_liquid_glass_image(key: CapsuleCacheKey, dpi: f32) -> Option<RgbImage> {
    let w = key.phys_w;
    let h = key.phys_h;
    if w <= 2 || h <= 2 {
        return None;
    }

    let (br, bg, bb) = key.bg_rgb;
    let (cr, cg, cb) = key.fg_rgb;
    let is_dark = (br as u32 * 299 + bg as u32 * 587 + bb as u32 * 114) / 1000 < 135;

    let wf = w as f32;
    let hf = h as f32;
    let pad_x = 1.2 * dpi;
    let pad_y = 1.5 * dpi;
    let cx = (wf - 1.0) * 0.5;
    // При наведении капсула чуть проседает вниз, а при нажатии утапливается полностью
    let cy_offset = match key.state {
        ControlState::Normal => -0.40 * dpi,
        ControlState::Hovered => 0.25 * dpi,
        ControlState::Pressed => 0.65 * dpi,
    };
    let cy = (hf - 1.0) * 0.5 + cy_offset;
    let half_w = (wf * 0.5 - pad_x).max(1.0);
    let half_h = (hf * 0.5 - pad_y).max(1.0);
    let radius = (half_h * 0.92).clamp(4.0 * dpi, 13.0 * dpi);
    let rim_width = 2.1 * dpi;
    let shadow_spread = 2.4 * dpi;

    let mut pixels = Vec::with_capacity((w * h * 3) as usize);

    for py in 0..h {
        let yf = py as f32;
        let v = yf / (hf - 1.0).max(1.0);

        for px in 0..w {
            let xf = px as f32;
            let u = xf / (wf - 1.0).max(1.0);

            let dist = sdf_rounded_rect(xf, yf, cx, cy, half_w, half_h, radius);

            // Мягкая внешняя тень снизу капсулы (при наведении слабее, при нажатии исчезает)
            let shadow_dist = sdf_rounded_rect(xf, yf - 1.1 * dpi, cx, cy, half_w, half_h, radius);
            let shadow_mul = match key.state {
                ControlState::Normal => 1.0,
                ControlState::Hovered => 0.45,
                ControlState::Pressed => 0.0,
            };
            let shadow_alpha =
                if shadow_mul > 0.0 && shadow_dist > -0.5 * dpi && shadow_dist < shadow_spread {
                    ((shadow_spread - shadow_dist) / (shadow_spread + 0.5 * dpi)).powi(2)
                        * if is_dark { 0.36 } else { 0.14 }
                        * shadow_mul
                } else {
                    0.0
                };
            let mut out_r = mix_u8(br, 0, shadow_alpha);
            let mut out_g = mix_u8(bg, 4, shadow_alpha);
            let mut out_b = mix_u8(bb, 12, shadow_alpha);

            // 4x4 субпиксельное сглаживание (16x MSAA) вдоль изогнутого края капсулы
            let coverage = if dist < -1.0 {
                1.0
            } else if dist > 1.0 {
                0.0
            } else {
                let mut inside = 0;
                for sy in 0..4 {
                    let oy = yf - 0.375 + (sy as f32) * 0.25;
                    for sx in 0..4 {
                        let ox = xf - 0.375 + (sx as f32) * 0.25;
                        if sdf_rounded_rect(ox, oy, cx, cy, half_w, half_h, radius) <= 0.0 {
                            inside += 1;
                        }
                    }
                }
                inside as f32 / 16.0
            };

            if coverage > 0.0 {
                let top_sheen = (1.0 - v * 1.45).clamp(0.0, 1.0).powf(1.35);
                let bottom_bounce = ((v - 0.65) / 0.35).clamp(0.0, 1.0) * 0.35;
                let diag_caustic = ((1.0 - u * 0.55) * (1.0 - v * 0.65)).clamp(0.0, 1.0);

                let (mut gr, mut gg, mut gb) = if is_dark {
                    let mix_fg = match key.state {
                        ControlState::Normal => 0.76,
                        ControlState::Hovered => 0.54,
                        ControlState::Pressed => 0.38,
                    };
                    let base_r = mix_u8(br, cr, mix_fg);
                    let base_g = mix_u8(bg, cg, mix_fg);
                    let base_b = mix_u8(bb, cb, mix_fg);

                    let lift = match key.state {
                        ControlState::Normal => {
                            top_sheen * 0.22 + diag_caustic * 0.08 + bottom_bounce * 0.10
                        }
                        ControlState::Hovered => top_sheen * 0.10 + bottom_bounce * 0.06,
                        ControlState::Pressed => 0.03,
                    };
                    (
                        mix_u8(base_r, 225, lift),
                        mix_u8(base_g, 240, lift),
                        mix_u8(base_b, 255, lift * 1.12),
                    )
                } else {
                    let (wh_r, wh_g, wh_b) = match key.state {
                        ControlState::Normal => (0.68, 0.72, 0.78),
                        ControlState::Hovered => (0.36, 0.40, 0.48),
                        ControlState::Pressed => (0.18, 0.22, 0.28),
                    };
                    let mut base_r = mix_u8(br, 255, wh_r);
                    let mut base_g = mix_u8(bg, 255, wh_g);
                    let mut base_b = mix_u8(bb, 255, wh_b);
                    if key.state != ControlState::Normal {
                        let shade = if key.state == ControlState::Hovered {
                            0.08
                        } else {
                            0.15
                        };
                        base_r = mix_u8(base_r, 140, shade);
                        base_g = mix_u8(base_g, 160, shade);
                        base_b = mix_u8(base_b, 190, shade);
                    }

                    let frost = match key.state {
                        ControlState::Normal => {
                            top_sheen * 0.55 + diag_caustic * 0.18 + bottom_bounce * 0.16
                        }
                        ControlState::Hovered => top_sheen * 0.22 + bottom_bounce * 0.08,
                        ControlState::Pressed => 0.04,
                    };
                    (
                        mix_u8(base_r, 255, frost),
                        mix_u8(base_g, 255, frost),
                        mix_u8(base_b, 255, frost),
                    )
                };

                // Оптический блик Френеля вдоль скруглённой кромки
                if dist > -rim_width {
                    let rim_band = (1.0 - (-dist / rim_width)).clamp(0.0, 1.0).powf(1.5);
                    let nx = (xf - cx) / half_w.max(1.0);
                    let ny = (yf - cy) / half_h.max(1.0);
                    let lens_diag = ((-nx * 0.55 - ny * 0.85).max(0.0)
                        + (nx * 0.55 + ny * 0.85).max(0.0) * 0.55)
                        .clamp(0.18, 1.0);

                    if is_dark {
                        let rim_mul = match key.state {
                            ControlState::Normal => 0.68,
                            ControlState::Hovered => 0.50,
                            ControlState::Pressed => 0.36,
                        };
                        let rim_strength = rim_band * lens_diag * rim_mul;
                        gr = mix_u8(gr, 215, rim_strength);
                        gg = mix_u8(gg, 238, rim_strength);
                        gb = mix_u8(gb, 255, rim_strength);
                    } else if dist > -1.0 * dpi {
                        let edge_mul = match key.state {
                            ControlState::Normal => 0.42,
                            ControlState::Hovered => 0.56,
                            ControlState::Pressed => 0.66,
                        };
                        let edge_t = ((dist + 1.0 * dpi) / dpi).clamp(0.0, 1.0) * edge_mul;
                        gr = mix_u8(gr, 130, edge_t);
                        gg = mix_u8(gg, 160, edge_t);
                        gb = mix_u8(gb, 200, edge_t);
                    } else {
                        let gleam_mul = match key.state {
                            ControlState::Normal => 0.75,
                            ControlState::Hovered => 0.40,
                            ControlState::Pressed => 0.20,
                        };
                        let inner_gleam = rim_band * lens_diag * gleam_mul;
                        gr = mix_u8(gr, 255, inner_gleam);
                        gg = mix_u8(gg, 255, inner_gleam);
                        gb = mix_u8(gb, 255, inner_gleam);
                    }
                }

                out_r = mix_u8(out_r, gr, coverage);
                out_g = mix_u8(out_g, gg, coverage);
                out_b = mix_u8(out_b, gb, coverage);
            }

            pixels.push(out_r);
            pixels.push(out_g);
            pixels.push(out_b);
        }
    }

    RgbImage::new(&pixels, w, h, ColorDepth::Rgb8).ok()
}

/// Генерирует строгий плоский HiDPI-контрол «Базовый минималистичный (IDE)» (в духе Zed / VS Code)
/// с аккуратным радиусом 5px, матовой поверхностью и тонкой сглаженной 1px-рамкой (16x MSAA).
fn build_basic_flat_image(key: CapsuleCacheKey, dpi: f32) -> Option<RgbImage> {
    let w = key.phys_w;
    let h = key.phys_h;
    if w <= 2 || h <= 2 {
        return None;
    }

    let (br, bg, bb) = key.bg_rgb;
    let (cr, cg, cb) = key.fg_rgb;
    let is_dark = (br as u32 * 299 + bg as u32 * 587 + bb as u32 * 114) / 1000 < 135;

    let wf = w as f32;
    let hf = h as f32;
    let pad = 1.0 * dpi;
    let cx = (wf - 1.0) * 0.5;
    let cy_shift = match key.state {
        ControlState::Normal => 0.0,
        ControlState::Hovered => 0.45 * dpi,
        ControlState::Pressed => 0.85 * dpi,
    };
    let cy = (hf - 1.0) * 0.5 + cy_shift;
    let half_w = (wf * 0.5 - pad).max(1.0);
    let half_h = (hf * 0.5 - pad).max(1.0);
    let radius = (5.0 * dpi).min(half_h * 0.85).max(1.5 * dpi);
    let border_w = (1.15 * dpi).max(1.0);

    let (surf_r, surf_g, surf_b) = if is_dark {
        let t = match key.state {
            ControlState::Normal => 0.88,
            ControlState::Hovered => 0.58,
            ControlState::Pressed => 0.36,
        };
        (mix_u8(br, cr, t), mix_u8(bg, cg, t), mix_u8(bb, cb, t))
    } else {
        match key.state {
            ControlState::Normal => (
                mix_u8(cr, 255, 0.55),
                mix_u8(cg, 255, 0.55),
                mix_u8(cb, 255, 0.55),
            ),
            ControlState::Hovered => (
                mix_u8(br, 185, 0.18),
                mix_u8(bg, 198, 0.18),
                mix_u8(bb, 218, 0.18),
            ),
            ControlState::Pressed => (
                mix_u8(br, 150, 0.25),
                mix_u8(bg, 165, 0.25),
                mix_u8(bb, 190, 0.25),
            ),
        }
    };

    let (bord_r, bord_g, bord_b) = if is_dark {
        let add = match key.state {
            ControlState::Normal => 44,
            ControlState::Hovered => 58,
            ControlState::Pressed => 70,
        };
        (
            br.saturating_add(add),
            bg.saturating_add(add + 6),
            bb.saturating_add(add + 16),
        )
    } else {
        let sub = match key.state {
            ControlState::Normal => 34,
            ControlState::Hovered => 50,
            ControlState::Pressed => 64,
        };
        (
            br.saturating_sub(sub),
            bg.saturating_sub(sub.saturating_sub(6)),
            bb.saturating_sub(sub.saturating_sub(14)),
        )
    };

    let mut pixels = Vec::with_capacity((w * h * 3) as usize);

    for py in 0..h {
        let yf = py as f32;
        for px in 0..w {
            let xf = px as f32;
            let dist = sdf_rounded_rect(xf, yf, cx, cy, half_w, half_h, radius);

            let coverage = if dist < -1.0 {
                1.0
            } else if dist > 1.0 {
                0.0
            } else {
                let mut inside = 0;
                for sy in 0..4 {
                    let oy = yf - 0.375 + (sy as f32) * 0.25;
                    for sx in 0..4 {
                        let ox = xf - 0.375 + (sx as f32) * 0.25;
                        if sdf_rounded_rect(ox, oy, cx, cy, half_w, half_h, radius) <= 0.0 {
                            inside += 1;
                        }
                    }
                }
                inside as f32 / 16.0
            };

            let mut out_r = br;
            let mut out_g = bg;
            let mut out_b = bb;

            if coverage > 0.0 {
                let (mut ir, mut ig, mut ib) = (surf_r, surf_g, surf_b);
                if dist > -border_w {
                    let bt = ((dist + border_w) / border_w).clamp(0.0, 1.0);
                    ir = mix_u8(ir, bord_r, bt);
                    ig = mix_u8(ig, bord_g, bt);
                    ib = mix_u8(ib, bord_b, bt);
                }
                out_r = mix_u8(out_r, ir, coverage);
                out_g = mix_u8(out_g, ig, coverage);
                out_b = mix_u8(out_b, ib, coverage);
            }

            pixels.push(out_r);
            pixels.push(out_g);
            pixels.push(out_b);
        }
    }

    RgbImage::new(&pixels, w, h, ColorDepth::Rgb8).ok()
}

fn draw_custom_control_box(x: i32, y: i32, w: i32, h: i32, col: Color, state: ControlState) {
    if w <= 2 || h <= 2 {
        return;
    }
    let dpi = app::screen_scale(0).clamp(1.0, 4.0);
    let phys_w = ((w as f32) * dpi).round().max(3.0) as i32;
    let phys_h = ((h as f32) * dpi).round().max(3.0) as i32;
    let skin = CURRENT_UI_SKIN.with(|s| s.get());
    let bg_rgb = CURRENT_PANEL_BG.with(|c| c.get());

    let key = CapsuleCacheKey {
        skin,
        phys_w,
        phys_h,
        fg_rgb: col.to_rgb(),
        bg_rgb,
        state,
    };

    let cached = GLASS_CACHE.with(|cache| {
        let mut map = cache.borrow_mut();
        if map.len() > 256 {
            map.clear();
        }
        map.entry(key)
            .or_insert_with(|| {
                let built = match skin {
                    UiSkin::LiquidGlass => build_liquid_glass_image(key, dpi),
                    UiSkin::BasicFlat => build_basic_flat_image(key, dpi),
                };
                let mut img = built.unwrap_or_else(|| {
                    RgbImage::new(&[bg_rgb.0, bg_rgb.1, bg_rgb.2], 1, 1, ColorDepth::Rgb8).unwrap()
                });
                // Привязываем к логическому размеру (w, h), сохраняя физический буфер phys_w × phys_h (1:1 HiDPI)
                img.scale(w, h, false, true);
                img
            })
            .clone()
    });

    let mut img = cached;
    img.draw(x, y, w, h);
}

fn draw_liquid_glass_up(x: i32, y: i32, w: i32, h: i32, col: Color) {
    draw_custom_control_box(x, y, w, h, col, ControlState::Normal);
}

fn draw_liquid_glass_hover(x: i32, y: i32, w: i32, h: i32, col: Color) {
    draw_custom_control_box(x, y, w, h, col, ControlState::Hovered);
}

fn draw_liquid_glass_down(x: i32, y: i32, w: i32, h: i32, col: Color) {
    draw_custom_control_box(x, y, w, h, col, ControlState::Pressed);
}

/// Привязывает семейство шрифта к 4 слотам FLTK (Regular, Bold, Italic, BoldItalic) с учётом ОС.
fn bind_fltk_font_quad(slots: [Font; 4], family_name: &str) {
    #[cfg(target_os = "windows")]
    {
        Font::set_font(slots[0], &format!(" {}", family_name));
        Font::set_font(slots[1], &format!("B{}", family_name));
        Font::set_font(slots[2], &format!("I{}", family_name));
        Font::set_font(slots[3], &format!("P{}", family_name));
    }
    #[cfg(not(target_os = "windows"))]
    {
        Font::set_font(slots[0], family_name);
        Font::set_font(slots[1], &format!("{} Bold", family_name));
        Font::set_font(slots[2], &format!("{} Italic", family_name));
        Font::set_font(slots[3], &format!("{} Bold Italic", family_name));
    }
}

/// Собирает список установленных в ОС шрифтов (плюс опциональную папку `./fonts/*.ttf`).
fn installed_font_set() -> &'static HashSet<String> {
    INSTALLED_FAMILIES_LOWER.get_or_init(|| {
        let mut set = HashSet::new();

        // Если рядом с проектом есть папка `fonts/`, подгружаем кастомные `.ttf` / `.otf` (например Zed Mono)
        let fonts_dir = Path::new("fonts");
        if let Ok(entries) = std::fs::read_dir(fonts_dir) {
            for entry in entries.flatten() {
                let p = entry.path();
                let is_font = p
                    .extension()
                    .and_then(|e| e.to_str())
                    .map(|ext| matches!(ext.to_ascii_lowercase().as_str(), "ttf" | "otf"))
                    .unwrap_or(false);
                if is_font && let Ok(loaded_name) = Font::load_font(&p) {
                    set.insert(loaded_name.trim().to_lowercase());
                }
            }
        }

        for raw in app::get_font_names() {
            #[cfg(target_os = "windows")]
            {
                if let Some(clean) = raw.strip_prefix(' ') {
                    set.insert(clean.trim().to_lowercase());
                }
            }
            #[cfg(not(target_os = "windows"))]
            {
                set.insert(raw.trim().to_lowercase());
            }
        }
        set
    })
}

fn pick_best_installed_font<'a>(candidates: &[&'a str], fallback: &'a str) -> &'a str {
    let installed = installed_font_set();
    for &cand in candidates {
        if installed.contains(&cand.to_lowercase()) {
            return cand;
        }
    }
    fallback
}

/// Переключает моноширинный шрифт кода (`Font::Courier..CourierBoldItalic`) на приятный современный шрифт.
pub fn bind_code_font_family(family: FontFamily) {
    let already_bound = LAST_BOUND_CODE_FAMILY.with(|c| c.get() == Some(family));
    if already_bound {
        return;
    }

    #[cfg(target_os = "windows")]
    let face = match family {
        FontFamily::ZedCascadia => pick_best_installed_font(
            &[
                "Zed Mono",
                "Zed Plex Mono",
                "JetBrains Mono",
                "Cascadia Mono",
                "Cascadia Code",
                "Consolas",
            ],
            "Consolas",
        ),
        FontFamily::JetBrainsFira => pick_best_installed_font(
            &[
                "JetBrains Mono",
                "Fira Code",
                "Cascadia Code",
                "Cascadia Mono",
                "Consolas",
            ],
            "Consolas",
        ),
        FontFamily::Consolas => {
            pick_best_installed_font(&["Consolas", "Cascadia Mono"], "Consolas")
        }
        FontFamily::CourierClassic => "Courier New",
    };

    #[cfg(not(target_os = "windows"))]
    let face = match family {
        FontFamily::ZedCascadia => pick_best_installed_font(
            &[
                "Zed Mono",
                "JetBrains Mono",
                "Cascadia Mono",
                "Fira Code",
                "Ubuntu Mono",
                "DejaVu Sans Mono",
            ],
            "Monospace",
        ),
        FontFamily::JetBrainsFira => pick_best_installed_font(
            &["JetBrains Mono", "Fira Code", "DejaVu Sans Mono"],
            "Monospace",
        ),
        FontFamily::Consolas => pick_best_installed_font(
            &["Consolas", "Liberation Mono", "DejaVu Sans Mono"],
            "Monospace",
        ),
        FontFamily::CourierClassic => {
            pick_best_installed_font(&["Courier New", "Liberation Mono"], "Monospace")
        }
    };

    bind_fltk_font_quad(
        [
            Font::Courier,
            Font::CourierBold,
            Font::CourierItalic,
            Font::CourierBoldItalic,
        ],
        face,
    );
    LAST_BOUND_CODE_FAMILY.with(|c| c.set(Some(family)));
}

/// Регистрирует современные системные шрифты (в духе Zed / Windows 11), HiDPI фильтрацию и SDF-шейдеры.
pub fn register_custom_frames() {
    Image::set_scaling_algorithm(RgbScaling::Bilinear);
    RgbImage::set_scaling_algorithm(RgbScaling::Bilinear);

    // Заменяем устаревший Microsoft Sans Serif на чёткий современный UI-шрифт (Zed Sans / Inter / Segoe UI)
    #[cfg(target_os = "windows")]
    let ui_face = pick_best_installed_font(&["Zed Sans", "Inter", "Segoe UI"], "Segoe UI");
    #[cfg(not(target_os = "windows"))]
    let ui_face = pick_best_installed_font(
        &["Zed Sans", "Inter", "Ubuntu", "Cantarell", "DejaVu Sans"],
        "Sans",
    );

    bind_fltk_font_quad(
        [
            Font::Helvetica,
            Font::HelveticaBold,
            Font::HelveticaItalic,
            Font::HelveticaBoldItalic,
        ],
        ui_face,
    );
    bind_code_font_family(FontFamily::ZedCascadia);

    app::set_frame_type_cb(LIQUID_GLASS_UP_BOX, draw_liquid_glass_up, 4, 2, 8, 4);
    app::set_frame_type_cb(LIQUID_GLASS_HOVER_BOX, draw_liquid_glass_hover, 4, 2, 8, 4);
    app::set_frame_type_cb(LIQUID_GLASS_DOWN_BOX, draw_liquid_glass_down, 4, 2, 8, 4);
}

/// Рисует фон шапки окна и статусбара: стеклянный градиент в Liquid Glass или матовый в Базовом IDE.
pub fn draw_glass_bar_background(
    x: i32,
    y: i32,
    w: i32,
    h: i32,
    panel_rgb: (u8, u8, u8),
    is_dark: bool,
    skin: UiSkin,
    bottom_border: bool,
) {
    if w <= 0 || h <= 0 {
        return;
    }
    if skin == UiSkin::BasicFlat {
        draw::set_draw_rgb_color(panel_rgb.0, panel_rgb.1, panel_rgb.2);
        draw::draw_rectf(x, y, w, h);
    } else {
        for row in 0..h {
            let t = row as f32 / (h - 1).max(1) as f32;
            let (r, g, b) = if is_dark {
                let lift = (1.0 - t) * 0.10;
                (
                    mix_u8(panel_rgb.0, 210, lift),
                    mix_u8(panel_rgb.1, 228, lift),
                    mix_u8(panel_rgb.2, 255, lift),
                )
            } else {
                let lift = (1.0 - t) * 0.45;
                (
                    mix_u8(panel_rgb.0, 255, lift),
                    mix_u8(panel_rgb.1, 255, lift),
                    mix_u8(panel_rgb.2, 255, lift),
                )
            };
            draw::set_draw_rgb_color(r, g, b);
            draw::draw_xyline(x, y + row, x + w - 1);
        }
    }

    let sep_y = if bottom_border { y + h - 1 } else { y };
    let (sr, sg, sb) = if is_dark {
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
    };
    draw::set_draw_rgb_color(sr, sg, sb);
    draw::draw_xyline(x, sep_y, x + w - 1);
}

/// Рисует аккуратную внешнюю рамку вокруг безрамочного окна.
pub fn draw_frameless_window_border(w: i32, h: i32, cfg: &EditorStyleConfig) {
    let chrome = cfg.chrome_palette();
    let (r, g, b) = if chrome.is_dark {
        (
            chrome.panel_bg.0.saturating_add(42),
            chrome.panel_bg.1.saturating_add(52),
            chrome.panel_bg.2.saturating_add(72),
        )
    } else {
        (
            chrome.panel_bg.0.saturating_sub(42),
            chrome.panel_bg.1.saturating_sub(32),
            chrome.panel_bg.2.saturating_sub(18),
        )
    };
    draw::set_draw_rgb_color(r, g, b);
    draw::draw_rect(0, 0, w, h);
}

/// На Windows включает скруглённые углы DWM и (для главного окна) иконку на панели задач и в Alt+Tab.
/// В C++ FLTK безрамочным окнам ставится `WS_EX_TOOLWINDOW`, который скрывает их из панели задач;
/// замена флага на `WS_EX_APPWINDOW` возвращает полноценную кнопку на панели задач Windows.
pub fn configure_frameless_os_window(win: &Window, show_in_taskbar: bool) {
    #[cfg(target_os = "windows")]
    {
        #[link(name = "dwmapi")]
        unsafe extern "system" {
            fn DwmSetWindowAttribute(
                hwnd: *mut std::ffi::c_void,
                dw_attribute: u32,
                pv_attribute: *const std::ffi::c_void,
                cb_attribute: u32,
            ) -> i32;
        }
        #[link(name = "user32")]
        unsafe extern "system" {
            fn GetWindowLongPtrW(hwnd: *mut std::ffi::c_void, index: i32) -> isize;
            fn SetWindowLongPtrW(hwnd: *mut std::ffi::c_void, index: i32, new_long: isize)
            -> isize;
            fn SetWindowPos(
                hwnd: *mut std::ffi::c_void,
                hwnd_insert_after: *mut std::ffi::c_void,
                x: i32,
                y: i32,
                cx: i32,
                cy: i32,
                flags: u32,
            ) -> i32;
        }

        const DWMWA_WINDOW_CORNER_PREFERENCE: u32 = 33;
        const DWMWCP_ROUND: i32 = 2;
        const GWL_STYLE: i32 = -16;
        const GWL_EXSTYLE: i32 = -20;
        const WS_SYSMENU: isize = 0x0008_0000;
        const WS_MINIMIZEBOX: isize = 0x0002_0000;
        const WS_EX_TOOLWINDOW: isize = 0x0000_0080;
        const WS_EX_APPWINDOW: isize = 0x0004_0000;
        const SWP_NOMOVE: u32 = 0x0002;
        const SWP_NOSIZE: u32 = 0x0001;
        const SWP_NOZORDER: u32 = 0x0004;
        const SWP_FRAMECHANGED: u32 = 0x0020;

        let hwnd = win.raw_handle();
        if !hwnd.is_null() {
            unsafe {
                let _ = DwmSetWindowAttribute(
                    hwnd,
                    DWMWA_WINDOW_CORNER_PREFERENCE,
                    (&DWMWCP_ROUND as *const i32).cast(),
                    std::mem::size_of::<i32>() as u32,
                );

                if show_in_taskbar {
                    let style = GetWindowLongPtrW(hwnd, GWL_STYLE);
                    SetWindowLongPtrW(hwnd, GWL_STYLE, style | WS_SYSMENU | WS_MINIMIZEBOX);

                    let ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
                    let new_ex = (ex_style & !WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW;
                    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, new_ex);

                    SetWindowPos(
                        hwnd,
                        std::ptr::null_mut(),
                        0,
                        0,
                        0,
                        0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED,
                    );
                }
            }
        }
    }
    #[cfg(not(target_os = "windows"))]
    {
        let _ = (win, show_in_taskbar);
    }
}

/// Возвращает наш кастомный HiDPI SDF FrameType (отрисовщик сам учитывает выбранный `UiSkin`).
pub fn button_frame_for_skin(_skin: UiSkin) -> FrameType {
    LIQUID_GLASS_UP_BOX
}

/// Обновляет глобальную палитру, шрифт кода и движок отрисовки FLTK под активную тему и скин.
pub fn apply_global_fltk_theme(cfg: &EditorStyleConfig) {
    bind_code_font_family(cfg.font_family);
    app::set_scheme(Scheme::Gtk);
    app::set_frame_border_radius_max(match cfg.ui_skin {
        UiSkin::LiquidGlass => 10,
        UiSkin::BasicFlat => 5,
    });
    app::set_frame_type2(FrameType::UpBox, LIQUID_GLASS_UP_BOX);
    app::set_frame_type2(FrameType::DownBox, LIQUID_GLASS_DOWN_BOX);

    let chrome = cfg.chrome_palette();
    CURRENT_UI_SKIN.with(|s| s.set(cfg.ui_skin));
    CURRENT_PANEL_BG.with(|c| c.set(chrome.panel_bg));
    GLASS_CACHE.with(|cache| cache.borrow_mut().clear());

    app::background(chrome.panel_bg.0, chrome.panel_bg.1, chrome.panel_bg.2);
    app::background2(
        chrome.surface_bg.0,
        chrome.surface_bg.1,
        chrome.surface_bg.2,
    );
    app::foreground(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);
    app::set_selection_color(
        chrome.accent_rgb.0,
        chrome.accent_rgb.1,
        chrome.accent_rgb.2,
    );
}

/// Стилизует кнопку в соответствии с текущей темой и добавляет мягкий эффект наведения с гарантированным возвратом.
pub fn style_button(btn: &mut Button, cfg: &EditorStyleConfig) {
    let chrome = cfg.chrome_palette();
    btn.set_frame(LIQUID_GLASS_UP_BOX);
    btn.set_down_frame(LIQUID_GLASS_DOWN_BOX);
    btn.set_color(Color::from_rgb(
        chrome.surface_bg.0,
        chrome.surface_bg.1,
        chrome.surface_bg.2,
    ));
    btn.set_selection_color(Color::from_rgb(
        chrome.accent_rgb.0,
        chrome.accent_rgb.1,
        chrome.accent_rgb.2,
    ));
    btn.set_label_color(Color::from_rgb(
        chrome.text_fg.0,
        chrome.text_fg.1,
        chrome.text_fg.2,
    ));

    let watch_running = Rc::new(Cell::new(false));
    btn.handle(move |b, ev| match ev {
        Event::Enter | Event::Move => {
            if b.frame() != LIQUID_GLASS_HOVER_BOX {
                b.set_frame(LIQUID_GLASS_HOVER_BOX);
                b.redraw();
            }
            if !watch_running.get() {
                watch_running.set(true);
                let flag = Rc::clone(&watch_running);
                let mut btn_w = b.clone();
                app::add_timeout3(0.06, move |handle| {
                    let Some(top_win) = btn_w.top_window() else {
                        btn_w.set_frame(LIQUID_GLASS_UP_BOX);
                        flag.set(false);
                        return;
                    };
                    if !top_win.shown() || !btn_w.visible_r() {
                        if btn_w.frame() != LIQUID_GLASS_UP_BOX {
                            btn_w.set_frame(LIQUID_GLASS_UP_BOX);
                            btn_w.redraw();
                        }
                        flag.set(false);
                        return;
                    }
                    let (mx_root, my_root) = app::get_mouse();
                    let mx = mx_root - top_win.x();
                    let my = my_root - top_win.y();
                    let inside = mx >= btn_w.x()
                        && mx < btn_w.x() + btn_w.w()
                        && my >= btn_w.y()
                        && my < btn_w.y() + btn_w.h();
                    if !inside {
                        if btn_w.frame() != LIQUID_GLASS_UP_BOX {
                            btn_w.set_frame(LIQUID_GLASS_UP_BOX);
                            btn_w.redraw();
                        }
                        flag.set(false);
                    } else {
                        app::repeat_timeout3(0.06, handle);
                    }
                });
            }
            true
        }
        Event::Leave | Event::Released | Event::Unfocus | Event::Deactivate | Event::Hide => {
            if b.frame() != LIQUID_GLASS_UP_BOX {
                b.set_frame(LIQUID_GLASS_UP_BOX);
                b.redraw();
            }
            true
        }
        _ => false,
    });
}

/// Стилизует выпадающий список `Choice` под активную палитру и UI-скин.
pub fn style_choice(choice: &mut Choice, cfg: &EditorStyleConfig) {
    let chrome = cfg.chrome_palette();
    let fg = Color::from_rgb(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);
    choice.set_frame(button_frame_for_skin(cfg.ui_skin));
    choice.set_color(Color::from_rgb(
        chrome.surface_bg.0,
        chrome.surface_bg.1,
        chrome.surface_bg.2,
    ));
    choice.set_text_color(fg);
    choice.set_label_color(fg);
    choice.set_selection_color(Color::from_rgb(
        chrome.accent_rgb.0,
        chrome.accent_rgb.1,
        chrome.accent_rgb.2,
    ));
}

/// Рендерит дорожку и круглый бегунок ползунка в физическом HiDPI-разрешении (16x MSAA) без артефактов.
fn build_hidpi_slider_track(
    phys_w: i32,
    phys_h: i32,
    dpi: f32,
    ratio: f32,
    panel_rgb: (u8, u8, u8),
    active_col: (u8, u8, u8),
    is_dark: bool,
    skin: UiSkin,
) -> Option<RgbImage> {
    if phys_w <= 6 || phys_h <= 6 {
        return None;
    }

    let wf = phys_w as f32;
    let hf = phys_h as f32;
    let cy = (hf - 1.0) * 0.5;
    let knob_r = (7.2 * dpi).min(hf * 0.44);
    let knob_r_sq = knob_r * knob_r;

    let margin_x = knob_r + 1.5 * dpi;
    let track_x0 = margin_x;
    let track_x1 = (wf - margin_x).max(track_x0 + 2.0);
    let track_cx = (track_x0 + track_x1) * 0.5;
    let track_half_w = (track_x1 - track_x0) * 0.5;
    let track_half_h = (3.0 * dpi).max(1.5);
    let track_r = track_half_h;

    let thumb_x = track_x0 + ratio.clamp(0.0, 1.0) * (track_x1 - track_x0);

    let groove_col = if is_dark {
        (
            panel_rgb.0.saturating_add(22),
            panel_rgb.1.saturating_add(26),
            panel_rgb.2.saturating_add(36),
        )
    } else {
        (
            panel_rgb.0.saturating_sub(24),
            panel_rgb.1.saturating_sub(18),
            panel_rgb.2.saturating_sub(10),
        )
    };

    let mut pixels = Vec::with_capacity((phys_w * phys_h * 3) as usize);

    for py in 0..phys_h {
        let yf = py as f32;
        for px in 0..phys_w {
            let xf = px as f32;
            let mut out = panel_rgb;

            // 1. Скруглённая горизонтальная дорожка ползунка (6px высотой)
            let t_dist =
                sdf_rounded_rect(xf, yf, track_cx, cy, track_half_w, track_half_h, track_r);
            if t_dist < 1.0 {
                let cov = if t_dist < -1.0 {
                    1.0
                } else {
                    let mut ins = 0;
                    for sy in 0..4 {
                        let oy = yf - 0.375 + (sy as f32) * 0.25;
                        for sx in 0..4 {
                            let ox = xf - 0.375 + (sx as f32) * 0.25;
                            if sdf_rounded_rect(
                                ox,
                                oy,
                                track_cx,
                                cy,
                                track_half_w,
                                track_half_h,
                                track_r,
                            ) <= 0.0
                            {
                                ins += 1;
                            }
                        }
                    }
                    ins as f32 / 16.0
                };
                let fill = if xf <= thumb_x {
                    active_col
                } else {
                    groove_col
                };
                out = (
                    mix_u8(out.0, fill.0, cov),
                    mix_u8(out.1, fill.1, cov),
                    mix_u8(out.2, fill.2, cov),
                );
            }

            // 2. Мягкая тень под круглым бегунком
            let sh_dx = xf - thumb_x;
            let sh_dy = yf - (cy + 1.1 * dpi);
            let sh_dist = (sh_dx * sh_dx + sh_dy * sh_dy).sqrt() - knob_r;
            if sh_dist > -0.5 * dpi && sh_dist < 2.2 * dpi {
                let sh_a = ((2.2 * dpi - sh_dist) / (2.7 * dpi)).powi(2)
                    * if is_dark { 0.38 } else { 0.18 };
                out = (
                    mix_u8(out.0, 0, sh_a),
                    mix_u8(out.1, 4, sh_a),
                    mix_u8(out.2, 12, sh_a),
                );
            }

            // 3. Круглый HiDPI-бегунок (16x MSAA)
            let dx = xf - thumb_x;
            let dy = yf - cy;
            let k_dist = (dx * dx + dy * dy).sqrt() - knob_r;
            if k_dist < 1.1 {
                let cov = if k_dist < -1.1 {
                    1.0
                } else {
                    let mut ins = 0;
                    for sy in 0..4 {
                        let oy = dy - 0.375 + (sy as f32) * 0.25;
                        for sx in 0..4 {
                            let ox = dx - 0.375 + (sx as f32) * 0.25;
                            if ox * ox + oy * oy <= knob_r_sq {
                                ins += 1;
                            }
                        }
                    }
                    ins as f32 / 16.0
                };

                if cov > 0.0 {
                    let (mut kr, mut kg, mut kb) = if is_dark {
                        (232, 240, 252)
                    } else {
                        (255, 255, 255)
                    };

                    if skin == UiSkin::LiquidGlass {
                        let spec = ((-dy / knob_r) * 0.6 + 0.3).clamp(0.0, 1.0).powi(2) * 0.25;
                        kr = mix_u8(kr, 255, spec);
                        kg = mix_u8(kg, 255, spec);
                        kb = mix_u8(kb, 255, spec);
                    }

                    // Тонкая контрастная кромка бегунка
                    if k_dist > -1.1 * dpi {
                        let rim_t = ((k_dist + 1.1 * dpi) / (1.1 * dpi)).clamp(0.0, 1.0) * 0.55;
                        let rim_col = if is_dark { active_col } else { (130, 155, 195) };
                        kr = mix_u8(kr, rim_col.0, rim_t);
                        kg = mix_u8(kg, rim_col.1, rim_t);
                        kb = mix_u8(kb, rim_col.2, rim_t);
                    }

                    out = (
                        mix_u8(out.0, kr, cov),
                        mix_u8(out.1, kg, cov),
                        mix_u8(out.2, kb, cov),
                    );
                }
            }

            pixels.push(out.0);
            pixels.push(out.1);
            pixels.push(out.2);
        }
    }

    RgbImage::new(&pixels, phys_w, phys_h, ColorDepth::Rgb8).ok()
}

/// Стилизует ползунок `HorValueSlider` с собственной чистой HiDPI-отрисовкой бегунка и шкалы.
pub fn style_slider(slider: &mut HorValueSlider, cfg: &EditorStyleConfig) {
    style_slider_accent(slider, cfg, None);
}

/// Стилизует ползунок `HorValueSlider` с возможностью задать индивидуальный цвет активной шкалы (например, R/G/B).
pub fn style_slider_accent(
    slider: &mut HorValueSlider,
    cfg: &EditorStyleConfig,
    custom_accent: Option<(u8, u8, u8)>,
) {
    let chrome = cfg.chrome_palette();
    let skin = cfg.ui_skin;
    let surf_col = Color::from_rgb(
        chrome.surface_bg.0,
        chrome.surface_bg.1,
        chrome.surface_bg.2,
    );
    let active_col = custom_accent.unwrap_or_else(|| {
        if chrome.is_dark {
            (
                mix_u8(chrome.accent_rgb.0, 120, 0.35).max(80),
                mix_u8(chrome.accent_rgb.1, 185, 0.35).max(130),
                mix_u8(chrome.accent_rgb.2, 255, 0.40).max(190),
            )
        } else {
            (59, 130, 246)
        }
    });

    slider.set_frame(FrameType::NoBox);
    slider.set_color(surf_col);
    slider.set_selection_color(Color::from_rgb(active_col.0, active_col.1, active_col.2));
    slider.set_text_color(Color::from_rgb(
        chrome.text_fg.0,
        chrome.text_fg.1,
        chrome.text_fg.2,
    ));

    slider.draw(move |sl| {
        let x = sl.x();
        let y = sl.y();
        let w = sl.w();
        let h = sl.h();
        if w <= 46 || h <= 6 {
            return;
        }

        // Полностью очищаем прямоугольник ползунка фоном панели (убирает любые артефакты частичной перерисовки)
        draw::set_draw_rgb_color(chrome.panel_bg.0, chrome.panel_bg.1, chrome.panel_bg.2);
        draw::draw_rectf(x, y, w, h);

        // 1. Аккуратная капсула числового значения слева
        let badge_w = 38;
        draw_custom_control_box(x, y, badge_w, h, surf_col, ControlState::Normal);
        let val_txt = format!("{}", sl.value().round() as i32);
        draw::set_font(Font::Helvetica, if val_txt.len() >= 3 { 12 } else { 13 });
        draw::set_draw_rgb_color(chrome.text_fg.0, chrome.text_fg.1, chrome.text_fg.2);
        draw::draw_text2(&val_txt, x, y, badge_w, h, Align::Center);

        // 2. Современная дорожка с круглым бегунком справа
        let track_x = x + badge_w + 6;
        let track_w = (w - badge_w - 6).max(12);
        let span = (sl.maximum() - sl.minimum()).abs().max(1e-6);
        let ratio = ((sl.value() - sl.minimum()) / span).clamp(0.0, 1.0) as f32;

        let dpi = app::screen_scale(0).clamp(1.0, 4.0);
        let phys_w = ((track_w as f32) * dpi).round().max(8.0) as i32;
        let phys_h = ((h as f32) * dpi).round().max(8.0) as i32;

        if let Some(mut img) = build_hidpi_slider_track(
            phys_w,
            phys_h,
            dpi,
            ratio,
            chrome.panel_bg,
            active_col,
            chrome.is_dark,
            skin,
        ) {
            img.scale(track_w, h, false, true);
            img.draw(track_x, y, track_w, h);
        }
    });
}
