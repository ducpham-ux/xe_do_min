"""Hàm tiện ích dựng file Word bằng python-docx (dùng chung cho Hướng dẫn và Báo cáo)."""
from docx import Document
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor

NAVY = RGBColor(0x1F, 0x3A, 0x5F)
BLUE = RGBColor(0x2E, 0x75, 0xB6)
FONT = "Arial"


class Doc:
    def __init__(self, footer_text):
        self.d = Document()
        sec = self.d.sections[0]
        sec.page_width, sec.page_height = Cm(21), Cm(29.7)
        sec.left_margin = sec.right_margin = Cm(2.2)
        sec.top_margin = sec.bottom_margin = Cm(2.0)
        st = self.d.styles
        self._font(st["Normal"], 11)
        st["Normal"].paragraph_format.space_after = Pt(6)
        st["Normal"].paragraph_format.line_spacing = 1.15
        for name, size, color, before, after in [("Heading 1", 16, NAVY, 18, 8), ("Heading 2", 13, BLUE, 12, 6), ("Heading 3", 11.5, NAVY, 10, 4)]:
            self._font(st[name], size, True, color)
            pf = st[name].paragraph_format
            pf.space_before, pf.space_after, pf.keep_with_next = Pt(before), Pt(after), True
        self._font(st["Title"], 24, True, NAVY)
        for s in ("List Bullet", "List Bullet 2"):
            self._font(st[s], 11)
            st[s].paragraph_format.space_after = Pt(3)
        self._footer(footer_text)

    @staticmethod
    def _font(style, size, bold=False, color=None):
        style.font.name = FONT
        style.font.size = Pt(size)
        style.font.bold = bold
        rpr = style.element.get_or_add_rPr()
        rf = rpr.find(qn("w:rFonts"))
        if rf is None:
            rf = OxmlElement("w:rFonts"); rpr.append(rf)
        for a in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
            rf.set(qn(a), FONT)
        if color is not None:
            style.font.color.rgb = color

    def _footer(self, text):
        p = self.d.sections[0].footer.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(text + "  ·  Trang ")
        r.font.size = Pt(8.5); r.font.color.rgb = RGBColor(0x7F, 0x7F, 0x7F)
        r2 = p.add_run(); r2.font.size = Pt(8.5)
        for typ, txt in (("begin", None), (None, "PAGE"), ("end", None)):
            if typ:
                e = OxmlElement("w:fldChar"); e.set(qn("w:fldCharType"), typ)
            else:
                e = OxmlElement("w:instrText"); e.set(qn("xml:space"), "preserve"); e.text = txt
            r2._r.append(e)

    # ------------------------------------------------------------ nội dung
    @staticmethod
    def runs(p, text, size=None, bold_all=False, color=None):
        parts = text.split("**")
        for i, part in enumerate(parts):
            if not part:
                continue
            # `code` -> Consolas
            sub = part.split("`")
            for j, sp in enumerate(sub):
                if not sp:
                    continue
                r = p.add_run(sp)
                r.bold = bold_all or (i % 2 == 1)
                if j % 2 == 1:
                    r.font.name = "Consolas"
                    r._element.rPr.rFonts.set(qn("w:hAnsi"), "Consolas")
                if size: r.font.size = Pt(size)
                if color is not None: r.font.color.rgb = color
        return p

    def h1(self, t): self.d.add_heading(t, level=1)
    def h2(self, t): self.d.add_heading(t, level=2)
    def h3(self, t): self.d.add_heading(t, level=3)

    def p(self, t, italic=False, size=None, align="j"):
        p = self.d.add_paragraph()
        self.runs(p, t, size=size)
        if italic:
            for r in p.runs: r.italic = True
        if align == "j": p.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
        elif align == "c": p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        return p

    def bullets(self, items, level=1):
        for t in items:
            self.runs(self.d.add_paragraph(style="List Bullet" if level == 1 else "List Bullet 2"), t)

    def steps(self, items):
        for i, t in enumerate(items, 1):
            p = self.d.add_paragraph()
            p.paragraph_format.left_indent = Cm(0.9); p.paragraph_format.first_line_indent = Cm(-0.6)
            p.paragraph_format.space_after = Pt(3)
            self.runs(p, f"{i}.  " + t)

    @staticmethod
    def shade(cell, hexcolor):
        tcPr = cell._tc.get_or_add_tcPr()
        shd = OxmlElement("w:shd"); shd.set(qn("w:val"), "clear"); shd.set(qn("w:color"), "auto"); shd.set(qn("w:fill"), hexcolor)
        tcPr.append(shd)

    def table(self, headers, rows, widths, fs=9):
        t = self.d.add_table(rows=1, cols=len(headers))
        t.style = "Table Grid"; t.alignment = WD_TABLE_ALIGNMENT.CENTER; t.autofit = False
        for i, h in enumerate(headers):
            c = t.rows[0].cells[i]; c.text = ""
            self.runs(c.paragraphs[0], h, size=fs, bold_all=True, color=RGBColor(0xFF, 0xFF, 0xFF))
            self.shade(c, "1F3A5F")
        for ri, row in enumerate(rows):
            cells = t.add_row().cells
            for i, v in enumerate(row):
                cells[i].text = ""
                lines = str(v).split("\n")
                self.runs(cells[i].paragraphs[0], lines[0], size=fs)
                for ln in lines[1:]:
                    self.runs(cells[i].add_paragraph(), ln, size=fs)
                for pp in cells[i].paragraphs:
                    pp.paragraph_format.space_after = Pt(1)
                if ri % 2 == 1:
                    self.shade(cells[i], "EEF3F9")
        for row in t.rows:
            for i, w in enumerate(widths):
                row.cells[i].width = Cm(w)
        trPr = t.rows[0]._tr.get_or_add_trPr(); th = OxmlElement("w:tblHeader"); th.set(qn("w:val"), "true"); trPr.append(th)
        self.d.add_paragraph().paragraph_format.space_after = Pt(2)

    def fig(self, path, caption, width=16):
        p = self.d.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.paragraph_format.keep_with_next = True
        p.add_run().add_picture(path, width=Cm(width))
        c = self.d.add_paragraph(); c.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = c.add_run(caption); r.italic = True; r.font.size = Pt(9.5); r.font.color.rgb = RGBColor(0x55, 0x55, 0x55)

    def code(self, text):
        for ln in text.strip("\n").split("\n"):
            p = self.d.add_paragraph()
            p.paragraph_format.space_after = Pt(0); p.paragraph_format.line_spacing = 1.0
            p.paragraph_format.left_indent = Cm(0.3)
            pPr = p._p.get_or_add_pPr(); shd = OxmlElement("w:shd"); shd.set(qn("w:val"), "clear"); shd.set(qn("w:fill"), "F2F2F2"); pPr.append(shd)
            r = p.add_run(ln if ln else " "); r.font.name = "Consolas"; r.font.size = Pt(8.5)
            r._element.rPr.rFonts.set(qn("w:hAnsi"), "Consolas")
        self.d.add_paragraph().paragraph_format.space_after = Pt(2)

    def note(self, text, fill="FFF2CC"):
        t = self.d.add_table(rows=1, cols=1); t.style = "Table Grid"; t.alignment = WD_TABLE_ALIGNMENT.CENTER
        c = t.rows[0].cells[0]; c.width = Cm(16.6); c.text = ""
        self.runs(c.paragraphs[0], text, size=10); self.shade(c, fill)
        self.d.add_paragraph().paragraph_format.space_after = Pt(2)

    def page_break(self):
        self.d.add_paragraph().add_run().add_break(WD_BREAK.PAGE)

    def toc(self):
        p = self.d.add_paragraph(); r = p.add_run()
        f1 = OxmlElement("w:fldChar"); f1.set(qn("w:fldCharType"), "begin")
        it = OxmlElement("w:instrText"); it.set(qn("xml:space"), "preserve"); it.text = 'TOC \\o "1-2" \\h \\z \\u'
        f2 = OxmlElement("w:fldChar"); f2.set(qn("w:fldCharType"), "separate")
        t = OxmlElement("w:t"); t.text = "Nhấn chuột phải → Update Field để hiện mục lục."
        f3 = OxmlElement("w:fldChar"); f3.set(qn("w:fldCharType"), "end")
        for e in (f1, it, f2, t, f3): r._r.append(e)

    def cover(self, kicker, title, subtitle, info_rows):
        for _ in range(5): self.d.add_paragraph()
        p = self.d.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        self.runs(p, kicker, size=11, bold_all=True, color=BLUE)
        p = self.d.add_paragraph(style="Title"); p.alignment = WD_ALIGN_PARAGRAPH.CENTER; p.add_run(title)
        p = self.d.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        self.runs(p, subtitle, size=13, color=NAVY)
        for _ in range(9): self.d.add_paragraph()
        self.table(["Mục", "Nội dung"], info_rows, [4, 12.6], fs=9.5)
        self.page_break()

    def save(self, path):
        self.d.save(path)
