# -*- coding: utf-8 -*-
import os
import sys
import csv
import shutil
import tempfile
import time

import matplotlib
import matplotlib.pyplot as plt

# 1. Определяем папку, где лежит сам скрипт, и путь к файлу с расчетами
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
csv_path = os.path.join(BASE_DIR, "result.csv")

if not os.path.exists(csv_path):
    print("[ERROR] Файл result.csv не найден!")
    sys.exit(1)

# 2. Читаем данные из таблицы result.csv в три списка
taus = []
us = []
ys = []

with open(csv_path, mode="r", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    for row in reader:
        taus.append(int(row["tau"]))
        us.append(float(row["u_tau"]))
        ys.append(float(row["y_tau"]))

# 3. Рисуем график: красным пунктиром — вход u, синей линией — выход y
fig = plt.figure(figsize=(9, 5))
plt.plot(taus, us, "r--o", label="Входной сигнал u(tau)", linewidth=1.5, markersize=4)
plt.plot(taus, ys, "b-s", label="Выход системы y(tau)", linewidth=2.0, markersize=4)

plt.title("Переходный процесс (ЛР №1, Вариант 7)", fontsize=12)
plt.xlabel("Шаг (tau)", fontsize=10)
plt.ylabel("Значение", fontsize=10)
plt.grid(True, linestyle=":", alpha=0.6)
plt.legend()
plt.tight_layout()

# 4. Сохраняем картинку в doc/latest_plot.png (для отчёта / readme)
doc_dir = os.path.join(BASE_DIR, "doc")
os.makedirs(doc_dir, exist_ok=True)
save_path = os.path.join(doc_dir, "latest_plot.png")

try:
    fig.savefig(save_path, dpi=300)
    print(f"График сохранен: {save_path}")
except PermissionError:
    # Файл занят другой программой (например, открыт в просмотрщике) —
    # сохраняем под уникальным именем, чтобы не падать.
    save_path = os.path.join(doc_dir, f"latest_plot_{int(time.time())}.png")
    fig.savefig(save_path, dpi=300)
    print(f"latest_plot.png занят, сохранено в: {save_path}")

# 5. Показываем график.
#    Основной способ — окно matplotlib: оно не зависит от просмотрщика фото
#    Windows ("Фотографии" зависает при повторном открытии того же файла).
backend = matplotlib.get_backend().lower()
if backend not in ("agg", "pdf", "svg", "ps", "cairo", "template"):
    plt.show()
else:
    # Запасной вариант: нет GUI-бэкенда — открываем КОПИЮ под новым именем,
    # чтобы просмотрщик каждый раз видел «новый» файл, а не закэшированный.
    plt.close(fig)
    if hasattr(os, "startfile"):
        tmp_path = os.path.join(tempfile.gettempdir(), f"plot_{int(time.time())}.png")
        shutil.copyfile(save_path, tmp_path)
        os.startfile(tmp_path)
