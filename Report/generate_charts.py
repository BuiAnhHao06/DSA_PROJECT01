#!/usr/bin/env python3
import csv
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CSV_PATH = ROOT / "benchmark_results.csv"
OUT_DIR = Path(__file__).resolve().parent

SERIES = [
    ("B-Tree Linear", "BTree", "linear", (0.10, 0.10, 0.10), "circle"),
    ("B-Tree Binary", "BTree", "binary", (0.80, 0.20, 0.20), "square"),
    ("B+ Tree Linear", "BPlus", "linear", (0.10, 0.35, 0.80), "triangle"),
    ("B+ Tree Binary", "BPlus", "binary", (0.10, 0.55, 0.25), "diamond"),
]


def pdf_escape(text):
    return text.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")


class PdfCanvas:
    def __init__(self, width=720, height=440):
        self.width = width
        self.height = height
        self.commands = []

    def color(self, rgb):
        r, g, b = rgb
        self.commands.append(f"{r:.3f} {g:.3f} {b:.3f} RG")
        self.commands.append(f"{r:.3f} {g:.3f} {b:.3f} rg")

    def linewidth(self, width):
        self.commands.append(f"{width:.2f} w")

    def line(self, x1, y1, x2, y2):
        self.commands.append(f"{x1:.2f} {y1:.2f} m {x2:.2f} {y2:.2f} l S")

    def polyline(self, points):
        if not points:
            return
        chunks = [f"{points[0][0]:.2f} {points[0][1]:.2f} m"]
        chunks.extend(f"{x:.2f} {y:.2f} l" for x, y in points[1:])
        chunks.append("S")
        self.commands.append(" ".join(chunks))

    def rect(self, x, y, w, h, fill=False):
        op = "f" if fill else "S"
        self.commands.append(f"{x:.2f} {y:.2f} {w:.2f} {h:.2f} re {op}")

    def circle(self, x, y, r, fill=True):
        # Four cubic Bezier arcs, using the standard kappa approximation.
        k = 0.5522847498 * r
        op = "f" if fill else "S"
        self.commands.append(
            f"{x + r:.2f} {y:.2f} m "
            f"{x + r:.2f} {y + k:.2f} {x + k:.2f} {y + r:.2f} {x:.2f} {y + r:.2f} c "
            f"{x - k:.2f} {y + r:.2f} {x - r:.2f} {y + k:.2f} {x - r:.2f} {y:.2f} c "
            f"{x - r:.2f} {y - k:.2f} {x - k:.2f} {y - r:.2f} {x:.2f} {y - r:.2f} c "
            f"{x + k:.2f} {y - r:.2f} {x + r:.2f} {y - k:.2f} {x + r:.2f} {y:.2f} c {op}"
        )

    def polygon(self, points, fill=True):
        if not points:
            return
        op = "f" if fill else "S"
        chunks = [f"{points[0][0]:.2f} {points[0][1]:.2f} m"]
        chunks.extend(f"{x:.2f} {y:.2f} l" for x, y in points[1:])
        chunks.append(f"h {op}")
        self.commands.append(" ".join(chunks))

    def marker(self, x, y, kind):
        r = 3.6
        if kind == "circle":
            self.circle(x, y, r)
        elif kind == "square":
            self.rect(x - r, y - r, 2 * r, 2 * r, fill=True)
        elif kind == "triangle":
            self.polygon([(x, y + r), (x - r, y - r), (x + r, y - r)])
        elif kind == "diamond":
            self.polygon([(x, y + r), (x - r, y), (x, y - r), (x + r, y)])

    def text(self, x, y, text, size=10, align="left"):
        safe = pdf_escape(text)
        # Helvetica's average glyph width is close enough for chart labels.
        width = len(text) * size * 0.52
        if align == "center":
            x -= width / 2
        elif align == "right":
            x -= width
        self.commands.append(f"BT /F1 {size:.1f} Tf {x:.2f} {y:.2f} Td ({safe}) Tj ET")

    def rotated_text(self, x, y, text, size=10):
        safe = pdf_escape(text)
        self.commands.append(f"BT /F1 {size:.1f} Tf 0 1 -1 0 {x:.2f} {y:.2f} Tm ({safe}) Tj ET")

    def save(self, path):
        stream = "\n".join(self.commands).encode("ascii")
        objects = [
            b"<< /Type /Catalog /Pages 2 0 R >>",
            b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
            (
                f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {self.width} {self.height}] "
                f"/Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>"
            ).encode("ascii"),
            b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
            b"<< /Length " + str(len(stream)).encode("ascii") + b" >>\nstream\n" + stream + b"\nendstream",
        ]

        output = bytearray(b"%PDF-1.4\n")
        offsets = [0]
        for index, obj in enumerate(objects, start=1):
            offsets.append(len(output))
            output.extend(f"{index} 0 obj\n".encode("ascii"))
            output.extend(obj)
            output.extend(b"\nendobj\n")
        xref = len(output)
        output.extend(f"xref\n0 {len(objects) + 1}\n".encode("ascii"))
        output.extend(b"0000000000 65535 f \n")
        for offset in offsets[1:]:
            output.extend(f"{offset:010d} 00000 n \n".encode("ascii"))
        output.extend(
            f"trailer << /Size {len(objects) + 1} /Root 1 0 R >>\n"
            f"startxref\n{xref}\n%%EOF\n".encode("ascii")
        )
        path.write_bytes(output)


def read_rows():
    with CSV_PATH.open(newline="") as f:
        return list(csv.DictReader(f))


def collect(rows, query_type):
    data = {}
    for label, style, search_type, _, _ in SERIES:
        points = []
        for row in rows:
            if row["query_type"] == query_type and row["style"] == style and row["search_type"] == search_type:
                points.append((int(row["N"]), float(row["mean_disk_reads"])))
        data[label] = sorted(points)
    return data


def log_map(value, source_min, source_max, target_min, target_max):
    lv = math.log10(value)
    lo = math.log10(source_min)
    hi = math.log10(source_max)
    return target_min + (lv - lo) / (hi - lo) * (target_max - target_min)


def draw_chart(path, title, data, y_min, y_max, y_ticks):
    canvas = PdfCanvas()
    left, bottom = 90, 78
    plot_w, plot_h = 455, 282
    right, top = left + plot_w, bottom + plot_h
    x_min, x_max = 1000, 10000000
    x_ticks = [1000, 10000, 100000, 1000000, 10000000]
    x_minor_ticks = [3000, 30000, 300000, 3000000]

    def sx(x):
        return log_map(x, x_min, x_max, left, right)

    def sy(y):
        return log_map(y, y_min, y_max, bottom, top)

    canvas.color((1, 1, 1))
    canvas.rect(0, 0, canvas.width, canvas.height, fill=True)

    canvas.color((0.88, 0.88, 0.88))
    canvas.linewidth(0.5)
    for tick in x_ticks + x_minor_ticks:
        x = sx(tick)
        canvas.line(x, bottom, x, top)
    for tick in y_ticks:
        y = sy(tick)
        canvas.line(left, y, right, y)

    canvas.color((0, 0, 0))
    canvas.linewidth(1.0)
    canvas.rect(left, bottom, plot_w, plot_h)
    for tick in x_ticks:
        x = sx(tick)
        canvas.line(x, bottom, x, bottom - 5)
        canvas.text(x, bottom - 21, f"{tick:g}", 8, "center")
    for tick in y_ticks:
        y = sy(tick)
        canvas.line(left - 5, y, left, y)
        label = f"{tick:g}"
        canvas.text(left - 10, y - 3, label, 8, "right")

    canvas.text((left + right) / 2, 404, title, 15, "center")
    canvas.text((left + right) / 2, 38, "Dataset size N (records)", 10, "center")
    canvas.rotated_text(30, (bottom + top) / 2 - 86, "Mean disk reads (pages/query)", 10)

    for label, _, _, color, marker in SERIES:
        points = [(sx(x), sy(y)) for x, y in data[label]]
        canvas.color(color)
        canvas.linewidth(1.8)
        canvas.polyline(points)
        for x, y in points:
            canvas.marker(x, y, marker)

    legend_x, legend_y = 565, 334
    canvas.color((0, 0, 0))
    canvas.text(legend_x, legend_y + 20, "Legend", 10)
    for i, (label, _, _, color, marker) in enumerate(SERIES):
        y = legend_y - i * 22
        canvas.color(color)
        canvas.linewidth(1.8)
        canvas.line(legend_x, y, legend_x + 28, y)
        canvas.marker(legend_x + 14, y, marker)
        canvas.color((0, 0, 0))
        canvas.text(legend_x + 38, y - 4, label, 9)

    canvas.text(left, 16, "Both axes use logarithmic scale.", 8)
    canvas.save(path)


def main():
    rows = read_rows()
    draw_chart(
        OUT_DIR / "point_query_disk_reads.pdf",
        "Point Query Disk Reads",
        collect(rows, "point"),
        1,
        10,
        [1, 2, 3, 4, 10],
    )
    draw_chart(
        OUT_DIR / "range_query_disk_reads.pdf",
        "Range Query Disk Reads",
        collect(rows, "range"),
        1,
        10000,
        [1, 10, 100, 1000, 10000],
    )


if __name__ == "__main__":
    main()
