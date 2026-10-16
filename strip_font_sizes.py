#!/usr/bin/env python3.12
"""
zNotes Helper Script: strip_font_sizes.py
Strips explicit point/pixel/em font-size inline CSS styles from zNotes HTML note files
(including <body> and inline spans) so text scales dynamically with application zooming.
"""

import sys
import re

def clean_html_font_sizes(html_content: str) -> str:
    # Match font-size properties with fixed/absolute units (e.g., 10pt, 16px, 1.2em)
    # Leaves relative HTML keywords untouched (x-small, small, medium, large, x-large, xx-large)
    pattern = r'font-size\s*:\s*\d+(\.\d+)?(pt|px|em|rem)\s*;?'
    cleaned = re.sub(pattern, '', html_content, flags=re.IGNORECASE)

    # Clean up empty style attributes left behind (e.g., style="" or style=" ")
    cleaned = re.sub(r'style="\s*"', '', cleaned)
    return cleaned

def main():
    if len(sys.argv) > 1:
        filepath = sys.argv[1]
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            cleaned = clean_html_font_sizes(content)
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(cleaned)
            print(f"Successfully cleaned font sizes in: {filepath}")
        except Exception as e:
            print(f"Error processing {filepath}: {e}", file=sys.stderr)
            sys.exit(1)
    else:
        content = sys.stdin.read()
        sys.stdout.write(clean_html_font_sizes(content))

if __name__ == '__main__':
    main()
