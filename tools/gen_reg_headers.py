import yaml
import sys

def parse_specfile(specfile):
    with open(specfile, 'r') as f:
        return yaml.load(f)

def reg_name_getter(reg, name, arg):
    return '{}_GET_{}({})'.format(reg, name, arg)

def print_getter(reg, name, offset, width):
    f = reg_name_getter(reg, name, 'v')
    f = '#define {:<40} BF_EXTRACT(v, {:<2}, {:<2})'.format(f, offset, width)
    print(f)

def print_setter(reg, name, offset, width):
    f = '{}_CLR_SET_{}(v, set)'.format(reg, name)
    f = '#define {:<48} BF_CLEAR_AND_SET(v, set, {:<2}, {:<2})'.format(f, offset, width)
    print(f)

def print_cleaner(reg, name, offset, width):
    f = '{}_CLR_{}(v)'.format(reg, name)
    f = '#define {:<40} BF_CLEAR(v, {:<2}, {:<2})'.format(f, offset, width)
    print(f)

def iter_bf(spec, reg, f):
    for name, offset, width in spec[reg]:
        f(reg, name, offset, width)

def print_printer(spec, reg):
    arg = 'v'
    fmt = ['%08x']
    values = [' ' * 4 + arg]
    for name, offset, width in spec[reg]:
        fmt.append('{}:%x'.format(name))
        values.append(' ' * 4 + '(int)' + reg_name_getter(reg, name, arg))
    fmt = ','.join(fmt)
    values = ',\n'.join(values)
    printer = ['static inline int {}_to_string(char *buf, int bufsz, uint32_t {})'.format(
            reg.lower(), arg)]
    printf = '  return snprintf(buf, bufsz, "{}"'.format(fmt)
    printf += ',\n' + values + ');'
    printer += ['{', printf,'}']
    printer = '\n'.join(printer)
    print('\n')
    print(printer)

def print_header():
    print('#pragma once\n')

def main(specfile):
    s = parse_specfile(specfile)
    print_header()
    for reg in s:
        iter_bf(s, reg, print_getter)
        iter_bf(s, reg, print_setter)
        iter_bf(s, reg, print_cleaner)
        print_printer(s, reg)

if __name__ == '__main__':
    specfile = sys.argv[1]
    main(specfile)

