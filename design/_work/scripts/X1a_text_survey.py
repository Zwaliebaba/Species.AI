#!/usr/bin/env python3
"""X1a: encoding / line-structure survey of text files.
Usage: python3 -I X1a_text_survey.py SRC_ROOT OUT_CSV REL_PATH...
For each file: size, encoding (BOM), line terminator mix, final-newline, 'redshirt2' header,
non-ASCII byte count, mid-line CR count, tab/space usage, number of lines, number of non-empty cleaned lines."""
import csv, os, sys

def decode(data):
    if data[:2] == b'\xff\xfe':
        return 'UTF-16LE+BOM', data[2:].decode('utf-16-le', errors='replace')
    if data[:2] == b'\xfe\xff':
        return 'UTF-16BE+BOM', data[2:].decode('utf-16-be', errors='replace')
    if data[:3] == b'\xef\xbb\xbf':
        return 'UTF-8+BOM(not recognised by reader)', data.decode('latin-1')
    try:
        data.decode('ascii')
        return 'ASCII', data.decode('latin-1')
    except UnicodeDecodeError:
        return 'Latin-1 bytes', data.decode('latin-1')

def clean(line):
    i = line.find('#')
    if i >= 0: line = line[:i]
    i = line.find('\n')
    if i >= 0: line = line[:i]
    i = line.find('\r')
    if i >= 0: line = line[:i]
    return line

def main():
    root, outcsv = sys.argv[1], sys.argv[2]
    rows = []
    for rel in sys.argv[3:]:
        p = os.path.join(root, rel)
        data = open(p, 'rb').read()
        enc, text = decode(data)
        lines = text.split('\n')
        last_unterminated = not text.endswith('\n') and len(text) > 0
        if text.endswith('\n'):
            lines = lines[:-1]
        crlf = sum(1 for l in lines if l.endswith('\r'))
        lf = len(lines) - crlf
        midcr = sum(1 for l in lines if '\r' in l.rstrip('\r'))
        nonascii = sum(1 for ch in text if ord(ch) > 127)
        enc_hdr = lines[0][:9].lower() == 'redshirt2' if lines else False
        nonempty = sum(1 for l in lines if clean(l).strip(' \t\r\n:,\f'))
        rows.append(dict(path=rel, bytes=len(data), encoding=enc, lines=len(lines), crlf=crlf, lf_only=lf,
                         mid_line_cr=midcr, unterminated_last_line=int(last_unterminated),
                         redshirt2=int(enc_hdr), non_ascii_chars=nonascii, nonempty_token_lines=nonempty))
    with open(outcsv, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader(); w.writerows(rows)
    for r in rows:
        flags = []
        if r['encoding'] != 'ASCII': flags.append(r['encoding'])
        if r['lf_only'] and r['crlf']: flags.append('mixed-EOL lf=%d' % r['lf_only'])
        elif r['lf_only']: flags.append('LF-only')
        if r['mid_line_cr']: flags.append('midCR=%d' % r['mid_line_cr'])
        if r['unterminated_last_line']: flags.append('NO-FINAL-LF')
        if r['redshirt2']: flags.append('ENCRYPTED')
        if r['non_ascii_chars'] and r['encoding'] == 'Latin-1 bytes': flags.append('nonascii=%d' % r['non_ascii_chars'])
        if flags: print(r['path'], '|', ', '.join(flags))
    print('files:', len(rows))

if __name__ == '__main__':
    main()
