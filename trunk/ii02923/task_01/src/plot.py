# -*- coding: utf-8 -*-
import os
import csv
import matplotlib.pyplot as plt

# Определяем константы для имен файлов во избежание дублирования литералов
CSV_NAME = "results.csv"
PLOT_NAME = "plot.png"

# Определяем абсолютный путь к папке src и к корневой папке ii02921
SRC_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SRC_DIR, ".."))

# CSV файл гарантированно ищется в корневой папке ii02921
csv_file = os.path.join(ROOT_DIR, CSV_NAME)

# Если файл еще не создан в корне, проверяем резервные пути (текущая директория или build)
if not os.path.exists(csv_file):
    csv_file = CSV_NAME
if not os.path.exists(csv_file):
    csv_file = os.path.join(ROOT_DIR, "build", "src", CSV_NAME)

# Папка doc гарантированно находится в корне ii02921/doc
doc_dir = os.path.join(ROOT_DIR, "doc")
if not os.path.exists(doc_dir):
    os.makedirs(doc_dir, exist_ok=True)

steps = []
inputs_u = []
outputs_y = []
model_title = "Selected Model"
signal_title = "Selected Signal"

with open(csv_file, 'r', encoding='utf-8') as f:
    reader = csv.DictReader(f)
    for r in reader:
        steps.append(int(r['step']))
        inputs_u.append(float(r['u']))
        outputs_y.append(float(r['y']))
        if 'model_name' in r and r['model_name']:
            model_title = r['model_name']
        if 'signal' in r and r['signal']:
            signal_title = r['signal']

# Построение графика
plt.figure(figsize=(9, 5))
plt.plot(steps, inputs_u, 'r--', label=f"Input Signal u ({signal_title})", linewidth=1.5)
plt.plot(steps, outputs_y, 'b-o', markersize=4, label=f"Output State y ({model_title})", linewidth=2)

plt.title(f"Dynamic Response: {model_title} | Input: {signal_title}", fontsize=12)
plt.xlabel("Step (tau)", fontsize=10)
plt.ylabel("Value", fontsize=10)
plt.grid(True, linestyle=':', alpha=0.6)
plt.legend()
plt.tight_layout()

# Сохранение графика ровно в ii02921/doc/plot.png
save_path = os.path.join(doc_dir, PLOT_NAME)
plt.savefig(save_path, dpi=300)
plt.close()

# Автоматическое открытие картинки после генерации
if hasattr(os, "startfile"):
    os.startfile(os.path.abspath(save_path))