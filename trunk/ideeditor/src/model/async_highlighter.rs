use crate::model::lexer::highlight_rust_to_style_string;
use std::collections::BTreeMap;
use std::sync::mpsc::{self, Receiver, Sender};
use std::thread::{self, JoinHandle};

/// Задание на фоновый лексический разбор текста одной вкладки.
#[derive(Debug, Clone)]
pub struct HighlightRequest {
    /// Внутренний идентификатор вкладки документа (`Document.id`)
    pub doc_id: usize,
    /// Монотонный номер ревизии документа на момент отправки текста
    pub revision: u64,
    /// Сырой UTF-8 текст документа для расчёта буфера стилей
    pub text: String,
}

/// Результат работы фонового лексера, возвращаемый в главный GUI-поток.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct HighlightResponse {
    /// Внутренний идентификатор вкладки документа
    pub doc_id: usize,
    /// Номер ревизии, для которой была рассчитана эта строка стилей
    pub revision: u64,
    /// Готовая ASCII-строка индексов стилей (`'A'..='F'`), байт-в-байт равная длине текста
    pub style_string: String,
}

/// Менеджер фонового потока подсветки синтаксиса (Worker Thread + `std::sync::mpsc`).
/// Не зависит от библиотеки `fltk` — соблюдает строгую изоляцию слоя `model`.
pub struct BackgroundHighlighter {
    /// Канал отправки заданий в рабочий поток (`Option` нужен для чистого закрытия в `Drop`)
    req_tx: Option<Sender<HighlightRequest>>,
    /// Канал получения готовых буферов стилей из рабочего потока
    resp_rx: Receiver<HighlightResponse>,
    /// Дескриптор фонового потока ОС
    worker: Option<JoinHandle<()>>,
}

impl BackgroundHighlighter {
    /// Запускает фоновый рабочий поток лексического анализатора.
    /// Функция `on_ready` вызывается из рабочего потока сразу после помещения ответа в канал
    /// (в GUI-режиме через неё отправляется сигнал пробуждения цикла событий `AppMsg::HighlightReady`).
    pub fn new<F>(on_ready: F) -> Self
    where
        F: Fn() + Send + 'static,
    {
        let (req_tx, req_rx) = mpsc::channel::<HighlightRequest>();
        let (resp_tx, resp_rx) = mpsc::channel::<HighlightResponse>();

        let worker = thread::Builder::new()
            .name("ideeditor-lexer-worker".to_string())
            .spawn(move || {
                Self::worker_loop(req_rx, resp_tx, on_ready);
            })
            .expect("Не удалось запустить фоновый поток лексера");

        Self {
            req_tx: Some(req_tx),
            resp_rx,
            worker: Some(worker),
        }
    }

    /// Основной цикл фонового потока с коалесценцией (схлопыванием) промежуточных ревизий.
    fn worker_loop<F>(
        req_rx: Receiver<HighlightRequest>,
        resp_tx: Sender<HighlightResponse>,
        on_ready: F,
    ) where
        F: Fn() + Send + 'static,
    {
        // Блокируемся на `recv()` без нагрузки на CPU, пока пользователь не изменит текст
        while let Ok(first_req) = req_rx.recv() {
            // Если пока шёл предыдущий разбор скопилась пачка быстрых нажатий клавиш,
            // оставляем только самую свежую ревизию для каждого `doc_id`.
            let mut latest_by_doc: BTreeMap<usize, HighlightRequest> = BTreeMap::new();
            latest_by_doc.insert(first_req.doc_id, first_req);

            for pending in req_rx.try_iter() {
                match latest_by_doc.get(&pending.doc_id) {
                    Some(existing) if existing.revision > pending.revision => {}
                    _ => {
                        latest_by_doc.insert(pending.doc_id, pending);
                    }
                }
            }

            for (_doc_id, req) in latest_by_doc {
                let style_string = highlight_rust_to_style_string(&req.text);
                let response = HighlightResponse {
                    doc_id: req.doc_id,
                    revision: req.revision,
                    style_string,
                };
                if resp_tx.send(response).is_err() {
                    return;
                }
                on_ready();
            }
        }
    }

    /// Неблокирующе отправляет текст документа в фоновый поток лексера.
    pub fn submit(&self, request: HighlightRequest) {
        if let Some(tx) = &self.req_tx {
            let _ = tx.send(request);
        }
    }

    /// Неблокирующе извлекает следующий готовый результат разметки (если он уже посчитан).
    pub fn try_recv(&self) -> Option<HighlightResponse> {
        self.resp_rx.try_recv().ok()
    }
}

impl Drop for BackgroundHighlighter {
    fn drop(&mut self) {
        // Закрываем передатчик `Sender` — `req_rx.recv()` в рабочем потоке вернёт `Err`,
        // после чего фоновый поток корректно завершится без утечек ресурсов.
        self.req_tx.take();
        if let Some(handle) = self.worker.take() {
            let _ = handle.join();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::{Arc, Condvar, Mutex};
    use std::time::Duration;

    #[test]
    fn worker_computes_accurate_rust_styles_off_thread() {
        let pair = Arc::new((Mutex::new(false), Condvar::new()));
        let pair_bg = Arc::clone(&pair);

        let highlighter = BackgroundHighlighter::new(move || {
            let (lock, cvar) = &*pair_bg;
            *lock.lock().unwrap() = true;
            cvar.notify_all();
        });

        let sample = "fn main() {\n    let x: i32 = 42; // тест UTF-8\n}\n";
        let expected = highlight_rust_to_style_string(sample);

        highlighter.submit(HighlightRequest {
            doc_id: 7,
            revision: 1,
            text: sample.to_string(),
        });

        let (lock, cvar) = &*pair;
        let guard = lock.lock().unwrap();
        let _ = cvar
            .wait_timeout_while(guard, Duration::from_secs(2), |ready| !*ready)
            .unwrap();

        let resp = highlighter
            .try_recv()
            .expect("Ответ от фонового лексера должен прийти в канал");
        assert_eq!(resp.doc_id, 7);
        assert_eq!(resp.revision, 1);
        assert_eq!(resp.style_string, expected);
        assert_eq!(resp.style_string.len(), sample.len());
    }

    #[test]
    fn burst_of_revisions_delivers_latest_revision_cleanly() {
        let pair = Arc::new((Mutex::new(0usize), Condvar::new()));
        let pair_bg = Arc::clone(&pair);

        let highlighter = BackgroundHighlighter::new(move || {
            let (lock, cvar) = &*pair_bg;
            *lock.lock().unwrap() += 1;
            cvar.notify_all();
        });

        let final_code = "pub struct Point { pub x: f64, pub y: f64 }";
        for rev in 1..=10 {
            let txt = if rev < 10 {
                format!("let step = {};", rev)
            } else {
                final_code.to_string()
            };
            highlighter.submit(HighlightRequest {
                doc_id: 1,
                revision: rev,
                text: txt,
            });
        }

        // Дожидаемся обработки пачки и забираем самый последний ответ
        let deadline = std::time::Instant::now() + Duration::from_secs(2);
        let mut last_resp: Option<HighlightResponse> = None;
        while std::time::Instant::now() < deadline {
            while let Some(r) = highlighter.try_recv() {
                last_resp = Some(r);
            }
            if matches!(&last_resp, Some(r) if r.revision == 10) {
                break;
            }
            let (lock, cvar) = &*pair;
            let guard = lock.lock().unwrap();
            let _ = cvar.wait_timeout(guard, Duration::from_millis(25));
        }

        let resp = last_resp.expect("Должна прийти финальная ревизия 10");
        assert_eq!(resp.revision, 10);
        assert_eq!(
            resp.style_string,
            highlight_rust_to_style_string(final_code)
        );
    }

    #[test]
    fn dropping_highlighter_stops_worker_without_panic() {
        let highlighter = BackgroundHighlighter::new(|| {});
        highlighter.submit(HighlightRequest {
            doc_id: 1,
            revision: 1,
            text: "let ok = true;".to_string(),
        });
        drop(highlighter);
    }
}
