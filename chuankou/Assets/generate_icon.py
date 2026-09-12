"""Rasterize the simple SVG icon into a multi-size Windows ICO."""

from pathlib import Path
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw


ASSETS = Path(__file__).resolve().parent
SVG = ASSETS / "clock-icon.svg"
ICO = ASSETS / "clock-icon.ico"
PREVIEW = ASSETS / "clock-icon-preview.png"
CANVAS = 256
SCALE = 4


def color(value):
    return None if value in (None, "none") else value


def point(element, name):
    return round(float(element.attrib[name]) * SCALE)


def main():
    root = ET.parse(SVG).getroot()
    image = Image.new("RGBA", (CANVAS * SCALE, CANVAS * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    for element in root:
        tag = element.tag.rsplit("}", 1)[-1]
        fill = color(element.get("fill"))
        stroke = color(element.get("stroke"))
        width = round(float(element.get("stroke-width", "1")) * SCALE)

        if tag == "rect":
            x, y = point(element, "x"), point(element, "y")
            w, h = point(element, "width"), point(element, "height")
            radius = point(element, "rx")
            draw.rounded_rectangle((x, y, x + w - 1, y + h - 1), radius, fill)
        elif tag == "circle":
            cx, cy, radius = (point(element, name) for name in ("cx", "cy", "r"))
            box = (cx - radius, cy - radius, cx + radius, cy + radius)
            draw.ellipse(box, fill=fill, outline=stroke, width=width)
        elif tag == "line":
            start = (point(element, "x1"), point(element, "y1"))
            end = (point(element, "x2"), point(element, "y2"))
            draw.line((start, end), fill=stroke, width=width)
            if element.get("stroke-linecap") == "round":
                radius = width // 2
                for x, y in (start, end):
                    draw.ellipse((x - radius, y - radius, x + radius, y + radius),
                                 fill=stroke)
        else:
            raise ValueError(f"Unsupported SVG element: {tag}")

    preview = image.resize((CANVAS, CANVAS), Image.Resampling.LANCZOS)
    preview.save(PREVIEW)
    preview.save(ICO, format="ICO", sizes=[(size, size) for size in
                                          (16, 24, 32, 48, 64, 128, 256)])


if __name__ == "__main__":
    main()
