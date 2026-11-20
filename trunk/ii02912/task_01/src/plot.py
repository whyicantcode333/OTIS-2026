"""Построение графика одного запуска: python plot.py имя_файла.csv
Файл ищется только в папке results текущего каталога."""
import csv
import os
import re
import sys

import matplotlib.pyplot as plt

RESULTS_DIR = os.path.realpath("results")


def safe_csv_path(arg):
    """Оставляет только имя файла и проверяет, что он внутри папки results."""
    name = os.path.basename(arg)
    if not re.fullmatch(r"[A-Za-z0-9_.-]+\.csv", name):
        sys.exit("Недопустимое имя файла: " + name)
    path = os.path.realpath(os.path.join(RESULTS_DIR, name))
    if os.path.commonpath([RESULTS_DIR, path]) != RESULTS_DIR:
        sys.exit("Файл вне папки results")
    return path


def main():
    if len(sys.argv) < 2:
        sys.exit("Использование: python plot.py имя_файла.csv")
    path = safe_csv_path(sys.argv[1])

    t, u, y = [], [], []
    with open(path, encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            t.append(int(row["tau"]))
            u.append(float(row["u"]))
            y.append(float(row["y"]))

    title = os.path.splitext(os.path.basename(path))[0]
    fig, ax = plt.subplots(figsize=(10, 5.5))
    ax.step(t, u, where="post", color="gray", alpha=.7, label="u(τ)")
    ax.plot(t, y, "o-", ms=4, label="y(τ)")
    ax.set_title(title)
    ax.set_xlabel("τ")
    ax.grid(True)
    ax.legend()
    fig.tight_layout()
    png = os.path.splitext(path)[0] + ".png"
    fig.savefig(png, dpi=120)
    print("График сохранён:", png)
    plt.show()


if __name__ == "__main__":
    main()
