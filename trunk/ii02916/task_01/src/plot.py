import csv
import matplotlib.pyplot as plt

# Списки для хранения данных из CSV
steps = []
inputs = []
outputs = []

# Чтение данных из файла result.csv, который создала программа на C++
try:
    with open('result.csv', mode='r', encoding='utf-8') as file:
        reader = csv.DictReader(file)
        for row in reader:
            steps.append(int(row['Step']))
            inputs.append(float(row['Input']))
            outputs.append(float(row['Output']))
except FileNotFoundError:
    print("[Ошибка] Файл result.csv не найден! Сначала запусти C++ программу.")
    exit(1)

# Настройка внешнего вида графика
plt.figure(figsize=(10, 6))

# Рисуем входной сигнал (пунктиром или тоньше)
plt.plot(steps, inputs, label='Input ($u_\\tau$)', color='orange', linestyle='--', linewidth=1.5)

# Рисуем выходной сигнал модели (жирной линией)
plt.plot(steps, outputs, label='Output ($y_\\tau$)', color='blue', linewidth=2)

# Подписи осей и заголовки
plt.title('Simulation Results (Variant 15)', fontsize=14)
plt.xlabel('Step ($\\tau$)', fontsize=12)
plt.ylabel('Value', fontsize=12)

# Включаем сетку и легенду
plt.axhline(0, color='black', linewidth=0.8, linestyle='--')
plt.grid(True, linestyle=':', alpha=0.7)
plt.legend(fontsize=12)

# Сохраняем график в картинку (для отчета)
plt.savefig('simulation_plot.png', dpi=300)
print("График успешно сохранен как simulation_plot.png!")

# Показываем график на экране
plt.show()