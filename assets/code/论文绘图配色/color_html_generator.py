import sys


def generate_color_divs(color_codes):
    div_elements = ""
    for code in color_codes:
        div_elements += f'  <div class="color-item" style="background-color: #{code}">#{code}</div>\n'

    return '<div class="color-set">\n' + div_elements + "</div>"


if __name__ == "__main__":
    print(generate_color_divs(sys.argv[1:]))
