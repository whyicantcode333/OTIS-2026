#!/usr/bin/env python3
"""Builds the 'Test coverage and results' section of readme.md (markdown, stdout) and,
optionally, a standalone HTML page with line and function coverage.
 
usage: python coverage_report.py coverage_summary.json gtest_report.xml [coverage_summary.html]
"""
import html
import json
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
 
 
def pct(entry, kind):
    value = entry.get(f"{kind}_percent")
    return "n/a" if value is None else f"{float(value):.1f} %"
 
 
def short_name(entry):
    return entry["filename"].replace("\\", "/").split("/")[-1]
 
 
BASE_DIR = Path.cwd().resolve()
 
 
def safe_path(raw):
    """Resolve a command-line path and refuse anything outside the working directory."""
    path = (BASE_DIR / raw).resolve()
    if not path.is_relative_to(BASE_DIR):
        raise ValueError(f"Path is outside the working directory: {raw}")
    return path
 
 
def load(summary_path, xml_path):
    with open(safe_path(summary_path), encoding="utf-8") as f:
        summary = json.load(f)
    root = ET.parse(safe_path(xml_path)).getroot()
    total = int(root.get("tests", 0))
    failed = int(root.get("failures", 0)) + int(root.get("errors", 0))
    passed = total - failed - int(root.get("disabled", 0))
    return summary, total, failed, passed
 
 
def print_markdown(summary, total, failed, passed):
    print("### Тестовое покрытие и результаты")
    print()
    print("**Способ запуска:** локальный скрипт `run_tests.bat`  ")
    print(f"**Всего модульных тестов:** {total}  ")
    print(f"**Успешно пройденных тестов:** {passed} / {total}")
    print()
    print("| Модуль / Класс | Покрытие строк (Line Coverage) | Покрытие функций (Func Coverage) | Статус |")
    print("|---|---|---|---|")
    for entry in sorted(summary.get("files", []), key=lambda e: e["filename"]):
        ok = float(entry.get("line_percent", 0)) >= 80.0
        print(f"| {short_name(entry)} | {pct(entry, 'line')} | {pct(entry, 'function')} | {'OK' if ok else 'Low'} |")
    overall = float(summary.get("line_percent", 0))
    status = "Success" if failed == 0 and overall >= 80.0 else "Fail"
    print(f"| **Итоговый результат** | **{pct(summary, 'line')}** | **{pct(summary, 'function')}** | **{status}** |")
 
 
def bar(entry, kind):
    value = entry.get(f"{kind}_percent")
    if value is None:
        return "n/a"
    value = float(value)
    covered, total = entry.get(f"{kind}_covered", "?"), entry.get(f"{kind}_total", "?")
    if value >= 90:
        color = "#2e9e4f"
    elif value >= 75:
        color = "#d9a400"
    else:
        color = "#d64545"
    return (f'<div class="cell"><div class="bar"><div style="width:{value:.1f}%;background:{color}"></div></div>'
            f'<span>{value:.1f} %</span><small>{covered} / {total}</small></div>')
 
 
def write_html(path, summary, total, failed, passed):
    rows = []
    for entry in sorted(summary.get("files", []), key=lambda e: e["filename"]):
        rows.append(f"<tr><td>{html.escape(short_name(entry))}</td><td>{bar(entry, 'line')}</td>"
                    f"<td>{bar(entry, 'function')}</td></tr>")
    rows.append(f'<tr class="total"><td>Итого</td><td>{bar(summary, "line")}</td><td>{bar(summary, "function")}</td></tr>')
    verdict = "все тесты пройдены" if failed == 0 else f"провалено тестов: {failed}"
    page = f"""<!doctype html>
<html lang="ru"><head><meta charset="utf-8"><title>Покрытие кода: ЛР №2</title>
<style>
body{{font-family:Segoe UI,Arial,sans-serif;margin:2rem auto;max-width:860px;color:#222}}
table{{border-collapse:collapse;width:100%}} th,td{{padding:.6rem .8rem;border-bottom:1px solid #ddd;text-align:left}}
th{{background:#f3f3f3}} .total td{{font-weight:600;background:#fafafa}}
.cell{{display:flex;align-items:center;gap:.6rem}} .bar{{width:140px;height:10px;background:#e6e6e6;border-radius:5px;overflow:hidden}}
.bar div{{height:100%}} small{{color:#777}}
</style></head><body>
<h1>Покрытие кода тестами</h1>
<p>Модульных тестов: <b>{total}</b>, успешно: <b>{passed} / {total}</b> ({verdict}).</p>
<table><tr><th>Модуль</th><th>Покрытие строк</th><th>Покрытие функций</th></tr>
{chr(10).join(rows)}
</table>
<p><small>Подробный отчёт по строкам исходного кода: <a href="coverage.html">coverage.html</a></small></p>
</body></html>
"""
    with open(safe_path(path), "w", encoding="utf-8") as f:
        f.write(page)
 
 
if __name__ == "__main__":
    if len(sys.argv) not in (3, 4):
        sys.exit(__doc__)
    data = load(sys.argv[1], sys.argv[2])
    print_markdown(*data)
    if len(sys.argv) == 4:
        write_html(sys.argv[3], *data)
