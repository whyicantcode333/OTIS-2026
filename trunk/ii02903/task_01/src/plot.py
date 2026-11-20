"""Visualization of the simulation results.
Usage:  python plot.py model13_step.csv [picture.png]
Shows the plots of u and y and a table of the values in ONE window.
If a second argument is given, the picture is also saved to that file.
"""
import csv
import math
import sys

try:
    import matplotlib.pyplot as plt
except ImportError:
    print("matplotlib is not installed. Run:  pip install matplotlib")
    sys.exit(1)

MAX_TABLE_ROWS = 30


def to_float(s):
    try:
        return float(s)
    except ValueError:
        return math.nan


def load(path):
    title = ""
    with open(path, newline="", encoding="utf-8") as f:
        lines = f.read().splitlines()
    if lines and lines[0].startswith("#"):
        title = lines[0][1:].strip()
        lines = lines[1:]
    rows = list(csv.DictReader(lines))
    t = [to_float(r["t"]) for r in rows]
    tau = list(range(len(rows)))
    u = [to_float(r["u"]) for r in rows]
    y = [to_float(r["y"]) for r in rows]
    return title, tau, t, u, y


def main():
    if len(sys.argv) < 2:
        print("Usage: python plot.py model13_step.csv [picture.png]")
        sys.exit(1)

    title, tau, t, u, y = load(sys.argv[1])
    discrete = all(abs(a - b) < 1e-12 for a, b in zip(t, tau))  # t = tau  ->  discrete model
    xs = tau if discrete else t
    xlabel = "tau (step)" if discrete else "t (time)"
    style = "o-" if len(xs) <= 60 else "-"

    fig = plt.figure(figsize=(14, 7))
    fig.suptitle(title, fontsize=10)
    gs = fig.add_gridspec(2, 2, width_ratios=[2, 1.1])

    ax_u = fig.add_subplot(gs[0, 0])
    ax_u.plot(xs, u, style, color="tab:orange", markersize=4)
    ax_u.set_ylabel("u")
    ax_u.set_title("Input signal")
    ax_u.grid(True)

    ax_y = fig.add_subplot(gs[1, 0], sharex=ax_u)
    ax_y.plot(xs, y, style, color="tab:blue", markersize=4)
    ax_y.set_ylabel("y")
    ax_y.set_xlabel(xlabel)
    ax_y.set_title("Model output")
    ax_y.grid(True)

    ax_t = fig.add_subplot(gs[:, 1])
    ax_t.axis("off")
    shown = min(len(xs), MAX_TABLE_ROWS)
    cells = [[str(tau[i]), f"{t[i]:.4g}", f"{u[i]:.4g}", f"{y[i]:.6g}"] for i in range(shown)]
    table = ax_t.table(cellText=cells, colLabels=["tau", "t", "u", "y"], loc="upper center", cellLoc="center")
    table.auto_set_font_size(False)
    table.set_fontsize(8)
    table.scale(1, 1.25)
    for (row, _), cell in table.get_celld().items():
        if row == 0:
            cell.set_text_props(weight="bold")
            cell.set_facecolor("#dddddd")
    note = f"first {shown} of {len(xs)} rows (all data: {sys.argv[1]})" if shown < len(xs) else f"{len(xs)} rows"
    ax_t.set_title("Table of values - " + note, fontsize=9)

    fig.tight_layout(rect=(0, 0, 1, 0.96))
    if len(sys.argv) > 2:
        fig.savefig(sys.argv[2], dpi=110)
    plt.show()


if __name__ == "__main__":
    main()
