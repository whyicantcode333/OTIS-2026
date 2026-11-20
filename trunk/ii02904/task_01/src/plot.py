import csv
from pathlib import Path

import matplotlib.pyplot as plt

PROJECT_DIR = Path(__file__).resolve().parent.parent
RESULT_FILE = "results.csv"
DOC_DIR = PROJECT_DIR / "doc"
MODEL_FILE_NAMES = {
    "Модель 1.4": "model14",
    "Модель 2.6": "model26",
    "Модель 3.8": "model38",
}
SIGNAL_FILE_NAMES = {
    "ступенчатый сигнал": "step",
    "импульсный сигнал": "impulse",
    "гармонический сигнал": "harmonic",
}


def main():
    with open(RESULT_FILE, encoding="utf-8") as file:
        title = file.readline().strip()
        rows = list(csv.DictReader(file))

    model_name, signal_name = title.split(", ", maxsplit=1)
    picture_name = f"{MODEL_FILE_NAMES[model_name]}_{SIGNAL_FILE_NAMES[signal_name]}.png"

    tau = [int(row["tau"]) for row in rows]
    u = [float(row["u"]) for row in rows]
    y = [float(row["y"]) for row in rows]

    plt.figure(figsize=(10, 6))
    plt.plot(tau, y, "o-", label="y (выход)")
    plt.plot(tau, u, "--", label="u (вход)")
    plt.title(title)
    plt.xlabel("Шаг tau")
    plt.grid(True)
    plt.legend()

    DOC_DIR.mkdir(parents=True, exist_ok=True)
    picture_path = DOC_DIR / picture_name
    plt.savefig(picture_path, dpi=150)
    print(f"График сохранён: {picture_path.relative_to(PROJECT_DIR)}")
    plt.show()


if __name__ == "__main__":
    main()
