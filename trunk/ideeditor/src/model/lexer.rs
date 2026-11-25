/// Категории синтаксических токенов (каждой соответствует символ в буфере стилей FLTK).
/// Модуль не зависит от GUI-библиотеки FLTK (чистая Domain-логика).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum TokenCategory {
    /// 'A' — Базовый текст, идентификаторы и операторы
    Plain = 0,
    /// 'B' — Ключевые слова языка (управляющие конструкции, декларации)
    Keyword = 1,
    /// 'C' — Типы данных, макросы, времена жизни и атрибуты #[...]
    TypeOrMacro = 2,
    /// 'D' — Строковые и символьные константы
    StringLiteral = 3,
    /// 'E' — Числовые константы (целые, дробные, hex/bin)
    NumberLiteral = 4,
    /// 'F' — Однострочные (//) и многострочные (/* ... */) комментарии
    Comment = 5,
}

impl TokenCategory {
    /// Все категории в порядке следования в таблице стилей FLTK ('A'..'F').
    pub const ALL: [TokenCategory; 6] = [
        TokenCategory::Plain,
        TokenCategory::Keyword,
        TokenCategory::TypeOrMacro,
        TokenCategory::StringLiteral,
        TokenCategory::NumberLiteral,
        TokenCategory::Comment,
    ];

    /// Символ стиля для буфера `HighlightBuf` ('A' + индекс в таблице).
    #[inline]
    pub fn style_byte(self) -> u8 {
        b'A' + (self as u8)
    }

    /// Название категории на русском языке для окна настройки стилей.
    pub fn display_name(self) -> &'static str {
        match self {
            TokenCategory::Plain => "Базовый текст и операторы",
            TokenCategory::Keyword => "Ключевые слова (fn, let, if...)",
            TokenCategory::TypeOrMacro => "Типы, макросы и атрибуты",
            TokenCategory::StringLiteral => "Строковые и символьные литералы",
            TokenCategory::NumberLiteral => "Числовые константы",
            TokenCategory::Comment => "Комментарии (// и /* ... */)",
        }
    }
}

const RUST_KEYWORDS: &[&str] = &[
    "as", "async", "await", "break", "const", "continue", "crate", "dyn", "else", "enum", "extern",
    "false", "fn", "for", "if", "impl", "in", "let", "loop", "match", "mod", "move", "mut", "pub",
    "ref", "return", "self", "static", "struct", "super", "trait", "true", "type", "unsafe", "use",
    "where", "while",
];

const RUST_BUILTIN_TYPES: &[&str] = &[
    "Self", "String", "Vec", "Option", "Result", "Some", "None", "Ok", "Err", "Box", "Rc", "Arc",
    "RefCell", "HashMap", "HashSet", "Path", "PathBuf", "bool", "char", "f32", "f64", "i8", "i16",
    "i32", "i64", "i128", "isize", "str", "u8", "u16", "u32", "u64", "u128", "usize",
];

/// Выполняет лексический анализ исходного текста Rust и формирует строку стилей `HighlightBuf`.
/// Длина возвращаемой строки в байтах строго равна `text.len()` (включая многобайтовую кириллицу UTF-8).
pub fn highlight_rust_to_style_string(text: &str) -> String {
    let bytes = text.as_bytes();
    let len = bytes.len();
    let mut out = vec![TokenCategory::Plain.style_byte(); len];

    let mut i = 0;
    while i < len {
        let b = bytes[i];

        // 1. Однострочный комментарий // ... до конца строки
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'/' {
            let start = i;
            i += 2;
            while i < len && bytes[i] != b'\n' {
                i += 1;
            }
            out[start..i].fill(TokenCategory::Comment.style_byte());
            continue;
        }

        // 2. Многострочный (в том числе вложенный) комментарий /* ... */
        if b == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
            let start = i;
            i += 2;
            let mut depth = 1usize;
            while i < len && depth > 0 {
                if bytes[i] == b'/' && i + 1 < len && bytes[i + 1] == b'*' {
                    depth += 1;
                    i += 2;
                } else if bytes[i] == b'*' && i + 1 < len && bytes[i + 1] == b'/' {
                    depth -= 1;
                    i += 2;
                } else {
                    i += 1;
                }
            }
            out[start..i].fill(TokenCategory::Comment.style_byte());
            continue;
        }

        // 3. Атрибуты #[...] и #![...]
        if b == b'#'
            && i + 1 < len
            && (bytes[i + 1] == b'['
                || (bytes[i + 1] == b'!' && i + 2 < len && bytes[i + 2] == b'['))
        {
            let start = i;
            let mut bracket_depth = 0usize;
            while i < len {
                if bytes[i] == b'[' {
                    bracket_depth += 1;
                } else if bytes[i] == b']' {
                    bracket_depth = bracket_depth.saturating_sub(1);
                    if bracket_depth == 0 {
                        i += 1;
                        break;
                    }
                }
                i += 1;
            }
            out[start..i].fill(TokenCategory::TypeOrMacro.style_byte());
            continue;
        }

        // 4. Сырые строковые литералы r"..." или r#"..."#
        if b == b'r' && i + 1 < len && (bytes[i + 1] == b'"' || bytes[i + 1] == b'#') {
            let mut hashes = 0usize;
            let mut cursor = i + 1;
            while cursor < len && bytes[cursor] == b'#' {
                hashes += 1;
                cursor += 1;
            }
            if cursor < len && bytes[cursor] == b'"' {
                let start = i;
                i = cursor + 1;
                while i < len {
                    if bytes[i] == b'"' {
                        let mut close_hashes = 0usize;
                        let mut k = i + 1;
                        while k < len && bytes[k] == b'#' && close_hashes < hashes {
                            close_hashes += 1;
                            k += 1;
                        }
                        if close_hashes == hashes {
                            i = k;
                            break;
                        }
                    }
                    i += 1;
                }
                out[start..i].fill(TokenCategory::StringLiteral.style_byte());
                continue;
            }
        }

        // 5. Обычные строковые литералы "..." с поддержкой экранирования \"
        if b == b'"' {
            let start = i;
            i += 1;
            while i < len {
                if bytes[i] == b'\\' {
                    i = (i + 2).min(len);
                } else if bytes[i] == b'"' {
                    i += 1;
                    break;
                } else {
                    i += 1;
                }
            }
            out[start..i].fill(TokenCategory::StringLiteral.style_byte());
            continue;
        }

        // 6. Символьные литералы ('a', '\n', 'я') VS времена жизни Rust ('static, 'a)
        if b == b'\'' {
            if let Some(end_char_lit) = try_scan_char_literal(text, i) {
                out[i..end_char_lit].fill(TokenCategory::StringLiteral.style_byte());
                i = end_char_lit;
                continue;
            } else if i + 1 < len && is_ident_start(bytes[i + 1]) {
                let start = i;
                i += 2;
                while i < len && is_ident_continue(bytes[i]) {
                    i += 1;
                }
                out[start..i].fill(TokenCategory::TypeOrMacro.style_byte());
                continue;
            }
        }

        // 7. Числовые литералы (42, 3.14, 0xFF, 0b1010, 1_000usize)
        if b.is_ascii_digit() {
            let start = i;
            i += 1;
            if b == b'0' && i < len && matches!(bytes[i], b'x' | b'X' | b'b' | b'B' | b'o' | b'O') {
                i += 1;
                while i < len && (bytes[i].is_ascii_hexdigit() || bytes[i] == b'_') {
                    i += 1;
                }
            } else {
                while i < len && (bytes[i].is_ascii_digit() || bytes[i] == b'_') {
                    i += 1;
                }
                if i + 1 < len && bytes[i] == b'.' && bytes[i + 1].is_ascii_digit() {
                    i += 1;
                    while i < len && (bytes[i].is_ascii_digit() || bytes[i] == b'_') {
                        i += 1;
                    }
                }
            }
            // Числовой суффикс типа (например, 10usize или 3.5f64)
            while i < len && is_ident_continue(bytes[i]) {
                i += 1;
            }
            out[start..i].fill(TokenCategory::NumberLiteral.style_byte());
            continue;
        }

        // 8. Идентификаторы, ключевые слова, типы и вызовы макросов (println!)
        if is_ident_start(b) {
            let start = i;
            i += 1;
            while i < len && is_ident_continue(bytes[i]) {
                i += 1;
            }
            let word = &text[start..i];

            // Если сразу за идентификатором идёт `!` (и это не `!=`), подсвечиваем как макрос
            if i < len && bytes[i] == b'!' && (i + 1 >= len || bytes[i + 1] != b'=') {
                i += 1;
                out[start..i].fill(TokenCategory::TypeOrMacro.style_byte());
                continue;
            }

            let category = if RUST_KEYWORDS.binary_search(&word).is_ok() {
                TokenCategory::Keyword
            } else if RUST_BUILTIN_TYPES.contains(&word)
                || word.chars().next().is_some_and(|c| c.is_ascii_uppercase())
            {
                TokenCategory::TypeOrMacro
            } else {
                TokenCategory::Plain
            };

            out[start..i].fill(category.style_byte());
            continue;
        }

        // 9. Прочие символы (пробелы, переводы строк, скобки, операторы, кириллица вне строк)
        i += 1;
    }

    // Все байты `out` гарантированно лежат в ASCII-диапазоне b'A'..=b'F'
    String::from_utf8(out).unwrap_or_default()
}

#[inline]
fn is_ident_start(b: u8) -> bool {
    b.is_ascii_alphabetic() || b == b'_'
}

#[inline]
fn is_ident_continue(b: u8) -> bool {
    b.is_ascii_alphanumeric() || b == b'_'
}

/// Проверяет, начинается ли в позиции `byte_idx` корректный символьный литерал Rust (`'x'`, `'\n'`, `'я'`).
/// Если да — возвращает индекс байта сразу после закрывающей кавычки `'`.
fn try_scan_char_literal(text: &str, byte_idx: usize) -> Option<usize> {
    let rest = text.get(byte_idx + 1..)?;
    let mut chars = rest.char_indices();
    let (_, first_ch) = chars.next()?;

    if first_ch == '\n' || first_ch == '\r' || first_ch == '\'' {
        return None;
    }

    if first_ch == '\\' {
        // Экранированный символ: '\n', '\'', '\x7f', '\u{1F600}'
        for (offset, ch) in chars {
            if ch == '\n' || ch == '\r' || offset > 12 {
                return None;
            }
            if ch == '\'' {
                return Some(byte_idx + 1 + offset + 1);
            }
        }
        None
    } else {
        // Одиночный Unicode-символ, сразу за которым обязана идти закрывающая кавычка `'`
        let (next_offset, second_ch) = chars.next()?;
        if second_ch == '\'' {
            Some(byte_idx + 1 + next_offset + 1)
        } else {
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn style_string_matches_utf8_byte_length_with_cyrillic() {
        let code = "// Привет, Rust!\nfn main() {\n    let s: &'static str = \"Текст\";\n    let n = 42;\n}\n";
        let styles = highlight_rust_to_style_string(code);
        assert_eq!(styles.len(), code.len());
    }

    #[test]
    fn distinguishes_keywords_strings_numbers_and_comments() {
        let code = "fn add() { let x = 123; \"hi\" /* блок */ }";
        let styles = highlight_rust_to_style_string(code);
        assert_eq!(styles.len(), code.len());

        // "fn" -> Keyword ('B')
        assert_eq!(&styles[0..2], "BB");
        // "123" -> NumberLiteral ('E')
        let num_pos = code.find("123").expect("number position");
        assert_eq!(&styles[num_pos..num_pos + 3], "EEE");
        // "\"hi\"" -> StringLiteral ('D')
        let str_pos = code.find("\"hi\"").expect("string position");
        assert_eq!(&styles[str_pos..str_pos + 4], "DDDD");
        // "/* блок */" -> Comment ('F')
        let cmt_pos = code.find("/*").expect("comment position");
        assert!(
            styles[cmt_pos..cmt_pos + "/* блок */".len()]
                .chars()
                .all(|c| c == 'F')
        );
    }

    #[test]
    fn lifetime_does_not_swallow_rest_of_file_as_string() {
        let code = "pub struct Doc { pub enc: &'static str }\nlet ok = 10;";
        let styles = highlight_rust_to_style_string(code);
        let let_pos = code.find("let").expect("let position");
        assert_eq!(&styles[let_pos..let_pos + 3], "BBB");
    }
}
