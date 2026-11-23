import json
import os
import sys
import xml.etree.ElementTree as ET
from datetime import datetime
from html import escape
from pathlib import Path

# Каталог проекта: читать и писать файлы разрешено только здесь
BASE_DIR = os.path.realpath(os.path.dirname(__file__))


def resolve_in_base(arg, allowed_suffixes):
    """Берёт из аргумента только имя файла и возвращает безопасный путь внутри BASE_DIR."""
    name = os.path.basename(arg)
    if not name or Path(name).suffix.lower() not in allowed_suffixes:
        raise ValueError(
            f"Недопустимое имя файла '{name}': ожидается расширение "
            f"{', '.join(sorted(allowed_suffixes))}."
        )

    full_path = os.path.realpath(os.path.join(BASE_DIR, name))
    if not full_path.startswith(BASE_DIR + os.sep):
        raise ValueError(f"Недопустимый путь к файлу: {name}")
    return Path(full_path)


def parse_cli_args(argv):
    """Разбирает аргументы командной строки по расширениям файлов."""
    xml_files = []
    json_arg = "coverage.json"
    out_arg = "test_report.html"

    for arg in argv:
        suffix = Path(arg).suffix.lower()
        if suffix == ".html":
            out_arg = arg
        elif suffix == ".json":
            json_arg = arg
        elif suffix == ".xml":
            xml_files.append(arg)
        else:
            raise ValueError(f"Недопустимое расширение файла '{arg}'.")

    xml_arg = xml_files[0] if len(xml_files) > 0 else "test_results.xml"
    cobertura_arg = xml_files[1] if len(xml_files) > 1 else "coverage.xml"
    return xml_arg, json_arg, cobertura_arg, out_arg


def parse_gtest_xml(xml_path):
    root = ET.parse(xml_path).getroot()
    total = int(root.attrib.get("tests", 0))
    failures = int(root.attrib.get("failures", 0))
    errors = int(root.attrib.get("errors", 0))
    time_sec = root.attrib.get("time", "0")
    passed = total - failures - errors

    cases = []
    for suite in root.findall("testsuite"):
        suite_name = suite.attrib.get("name", "")
        for case in suite.findall("testcase"):
            name = case.attrib.get("name", "")
            ctime = case.attrib.get("time", "0")
            fail_node = case.find("failure")
            status = "FAILED" if fail_node is not None else "PASSED"
            cases.append((suite_name, name, status, ctime))
    return total, passed, failures + errors, time_sec, cases


def summarize(rows_raw, source):
    """rows_raw: список (имя, покрыто строк, всего строк, покрыто функций, всего функций)."""
    if not rows_raw:
        return None

    rows = []
    tot_cov_l = tot_all_l = tot_cov_f = tot_all_f = 0
    for fname, cov_l, all_l, cov_f, all_f in rows_raw:
        line_pct = (cov_l / all_l * 100.0) if all_l else 100.0
        func_pct = (cov_f / all_f * 100.0) if all_f else 100.0
        rows.append((fname, line_pct, func_pct, "OK" if line_pct >= 80.0 else "LOW"))
        tot_cov_l += cov_l
        tot_all_l += all_l
        tot_cov_f += cov_f
        tot_all_f += all_f

    total_line_pct = (tot_cov_l / tot_all_l * 100.0) if tot_all_l else 100.0
    total_func_pct = (tot_cov_f / tot_all_f * 100.0) if tot_all_f else 100.0
    return rows, total_line_pct, total_func_pct, source


def parse_gcovr_json(cov_path):
    if not cov_path.is_file():
        return None
    with cov_path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    raw = []
    for file_info in data.get("files", []):
        fname = Path(file_info.get("file", "")).name
        if not fname.endswith(".cpp") or fname == "main.cpp":
            continue

        lines = [ln for ln in file_info.get("lines", []) if not ln.get("gcovr/noncode")]
        funcs = file_info.get("functions", [])
        raw.append((
            fname,
            sum(1 for ln in lines if ln.get("count", 0) > 0),
            len(lines),
            sum(1 for fn in funcs if fn.get("execution_count", 0) > 0),
            len(funcs),
        ))
    return summarize(raw, "gcovr")


def parse_cobertura_xml(cov_path):
    if not cov_path.is_file():
        return None
    root = ET.parse(cov_path).getroot()

    raw = []
    for cls in root.findall(".//class"):
        fname = Path(cls.attrib.get("filename", "")).name
        if not fname.endswith(".cpp") or fname == "main.cpp":
            continue

        lines = cls.findall("./lines/line")
        methods = cls.findall("./methods/method")
        raw.append((
            fname,
            sum(1 for ln in lines if int(ln.attrib.get("hits", 0)) > 0),
            len(lines),
            sum(1 for m in methods if float(m.attrib.get("line-rate", 0)) > 0),
            len(methods),
        ))
    return summarize(raw, "Cobertura XML")


def format_pct(val):
    if val is None:
        return "н/д"
    return f"{int(val)} %" if round(val) in (0, 100) and abs(val - round(val)) < 1e-6 else f"{val:.1f} %"


def build_coverage_block(cov_data):
    """Таблица покрытия. Если данных нет, честно сообщаем об этом (без выдуманных чисел)."""
    if cov_data is None:
        return (
            "<tr><td colspan='4' class='center'>Данные покрытия не найдены. "
            "Установите gcovr и соберите проект компилятором GCC (MinGW) с флагом "
            "<code>--coverage</code>.</td></tr>\n"
        ), None, None, "нет данных", "—"

    rows, total_line_pct, total_func_pct, source = cov_data
    overall = "Success" if total_line_pct >= 80.0 else "Warning"

    trs = ""
    for fname, line_pct, func_pct, st in rows:
        badge = '<span class="badge-pass">OK</span>' if st == "OK" else '<span class="badge-fail">LOW</span>'
        trs += (
            f"<tr><td><code>{escape(fname)}</code></td>"
            f"<td class='num'>{format_pct(line_pct)}</td>"
            f"<td class='num'>{format_pct(func_pct)}</td>"
            f"<td class='center'>{badge}</td></tr>\n"
        )
    trs += (
        f"<tr class='total-row'><td><strong>Итоговый результат</strong></td>"
        f"<td class='num'><strong>{format_pct(total_line_pct)}</strong></td>"
        f"<td class='num'><strong>{format_pct(total_func_pct)}</strong></td>"
        f"<td class='center'><strong style='color:#4ade80;'>{overall}</strong></td></tr>\n"
    )
    return trs, total_line_pct, total_func_pct, source, overall


def render_html(total, passed, failed, time_sec, cases, cov_data):
    cov_trs, total_line_pct, total_func_pct, cov_source, _ = build_coverage_block(cov_data)

    status_color = "#22c55e" if failed == 0 else "#ef4444"
    status_text = "ALL TESTS PASSED" if failed == 0 else "SOME TESTS FAILED"

    test_trs = ""
    for suite, name, status, ctime in cases:
        badge = '<span class="badge-pass">PASSED</span>' if status == "PASSED" else '<span class="badge-fail">FAILED</span>'
        test_trs += (
            f"<tr><td>{escape(suite)}</td><td>{escape(name)}</td>"
            f"<td class='center'>{badge}</td><td class='num'>{escape(ctime)} с</td></tr>\n"
        )

    return f"""<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<title>Google Test &amp; Coverage Report — task_02</title>
<style>
  body {{ font-family: 'Segoe UI', system-ui, sans-serif; background: #0f172a; color: #e2e8f0; margin: 0; padding: 32px; }}
  .container {{ max-width: 1050px; margin: 0 auto; }}
  .header {{ display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px; }}
  h1 {{ margin: 0; font-size: 26px; color: #38bdf8; }}
  .banner {{ padding: 8px 18px; border-radius: 999px; font-weight: 700; background: rgba(34,197,94,0.15); color: {status_color}; border: 1px solid {status_color}; }}
  .stats {{ display: grid; grid-template-columns: repeat(5, 1fr); gap: 14px; margin-bottom: 24px; }}
  .stat-box {{ background: #1e293b; padding: 16px; border-radius: 12px; border: 1px solid #334155; text-align: center; }}
  .stat-val {{ font-size: 24px; font-weight: 700; color: #f8fafc; }}
  .stat-lbl {{ font-size: 12px; color: #94a3b8; margin-top: 4px; }}
  .card {{ background: #1e293b; border-radius: 14px; padding: 24px; margin-bottom: 24px; border: 1px solid #334155; }}
  h2 {{ margin-top: 0; font-size: 18px; color: #f8fafc; display: flex; justify-content: space-between; align-items: center; }}
  .sub-tag {{ font-size: 12px; font-weight: 500; color: #94a3b8; background: #0f172a; padding: 4px 10px; border-radius: 6px; border: 1px solid #334155; }}
  table {{ width: 100%; border-collapse: collapse; font-size: 14px; }}
  th {{ background: #0f172a; color: #94a3b8; text-align: left; padding: 12px; border-bottom: 2px solid #334155; }}
  th.num, td.num {{ text-align: center; font-variant-numeric: tabular-nums; }}
  th.center, td.center {{ text-align: center; }}
  td {{ padding: 10px 12px; border-bottom: 1px solid #334155; }}
  tr:hover {{ background: #263349; }}
  tr.total-row {{ background: #162032; border-top: 2px solid #475569; }}
  code {{ background: #0f172a; padding: 2px 7px; border-radius: 5px; color: #7dd3fc; font-family: Consolas, monospace; }}
  .badge-pass {{ background: rgba(34,197,94,0.18); color: #4ade80; padding: 3px 10px; border-radius: 6px; font-weight: 600; font-size: 12px; }}
  .badge-fail {{ background: rgba(239,68,68,0.18); color: #f87171; padding: 3px 10px; border-radius: 6px; font-weight: 600; font-size: 12px; }}
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <div>
      <h1>Тестовое покрытие и результаты (Google Test &amp; Mock)</h1>
      <div style="color:#94a3b8; font-size:13px; margin-top:4px;">Способ запуска: <code>run_tests.bat</code> | Сгенерировано: {datetime.now().strftime("%Y-%m-%d %H:%M:%S")}</div>
    </div>
    <div class="banner">{status_text}</div>
  </div>

  <div class="stats">
    <div class="stat-box"><div class="stat-val">{total}</div><div class="stat-lbl">Всего тестов</div></div>
    <div class="stat-box"><div class="stat-val" style="color:#4ade80;">{passed}</div><div class="stat-lbl">Успешно</div></div>
    <div class="stat-box"><div class="stat-val" style="color:#38bdf8;">{format_pct(total_line_pct)}</div><div class="stat-lbl">Line Coverage</div></div>
    <div class="stat-box"><div class="stat-val" style="color:#a78bfa;">{format_pct(total_func_pct)}</div><div class="stat-lbl">Func Coverage</div></div>
    <div class="stat-box"><div class="stat-val" style="color:#fbbf24;">{escape(time_sec)} с</div><div class="stat-lbl">Время</div></div>
  </div>

  <div class="card">
    <h2>
      <span>Покрытие модулей тестами</span>
      <span class="sub-tag">Инструмент: {escape(cov_source)}</span>
    </h2>
    <table>
      <thead>
        <tr>
          <th>Модуль / Класс</th>
          <th class="num">Покрытие строк (Line Coverage)</th>
          <th class="num">Покрытие функций (Func Coverage)</th>
          <th class="center">Статус</th>
        </tr>
      </thead>
      <tbody>
        {cov_trs}
      </tbody>
    </table>
  </div>

  <div class="card">
    <h2>Детализация тестовых сценариев ({passed} / {total})</h2>
    <table>
      <thead>
        <tr>
          <th>Набор тестов (Test Suite)</th>
          <th>Тест (Test Case)</th>
          <th class="center">Статус</th>
          <th class="num">Время</th>
        </tr>
      </thead>
      <tbody>
        {test_trs}
      </tbody>
    </table>
  </div>
</div>
</body>
</html>
"""


def main():
    try:
        xml_arg, json_arg, cobertura_arg, out_arg = parse_cli_args(sys.argv[1:])
        xml_path = resolve_in_base(xml_arg, {".xml"})
        json_path = resolve_in_base(json_arg, {".json"})
        cobertura_path = resolve_in_base(cobertura_arg, {".xml"})
        out_path = resolve_in_base(out_arg, {".html"})
    except ValueError as error:
        print(f"Ошибка: {error}")
        return 1

    if not xml_path.is_file():
        print(f"Ошибка: файл с результатами тестов не найден: {xml_path.name}")
        return 1

    try:
        total, passed, failed, time_sec, cases = parse_gtest_xml(xml_path)
        cov_data = parse_gcovr_json(json_path) or parse_cobertura_xml(cobertura_path)
    except (ET.ParseError, OSError, ValueError) as error:
        print(f"Ошибка чтения отчётов: {error}")
        return 1

    page = render_html(total, passed, failed, time_sec, cases, cov_data)
    with out_path.open("w", encoding="utf-8") as f:
        f.write(page)
    print(f"Отчёт сохранён: {out_path.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())