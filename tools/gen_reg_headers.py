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

def print_bit_mask(reg, name, offset, width):
    f = '{}_MASK_{}'.format(reg, name)
    f = '#define {:<40} BF_MASK_AT_32({}, {})'.format(f, offset, width)
    print(f)

def print_bit_shift(reg, name, offset, width):
    f = '{}_SHIFT_{}'.format(reg, name)
    f = '#define {:<40} {}'.format(f, offset)
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

def gen_function_string(name, return_type, arguments, body):
    function_decl = '{storage_type} {return_type} {name}({arguments})'.format(
        storage_type = 'static inline',
        return_type = return_type,
        name = name,
        arguments = ', '.join(arguments))
    body = map(lambda x: '  ' + x, body)
    return '\n'.join(['', function_decl, '{', '\n'.join(body), '}'])

def print_bitmask_printer(spec, reg):
    value_argname = 'v'
    body = []
    arguments = ['char *buf', 'int bufsz', 'uint32_t {}'.format(value_argname)]
    function_name = '{}_bitmask_to_string'.format(reg.lower())

    body.append('int n = 0;')
    body.append('int first = 1;')
    for name, offset, width in spec[reg]:
        body += [
            'if ({}) '.format(reg_name_getter(reg, name, value_argname)) + '{',
            '  n += snprintf(buf + n, bufsz - n, "%s{}", first ? "" : ",");'.format(name),
            '  first = 0;',
            '}'
        ]
    body += ['return n;']
    print(gen_function_string(function_name, 'int', arguments, body))

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

def is_true_bitmask(spec, reg):
    for name, offset, width in spec[reg]:
        if width > 1:
            return False
    return True

def main(specfile):
    s = parse_specfile(specfile)
    print_header()
    for reg in s:
        iter_bf(s, reg, print_getter)
        iter_bf(s, reg, print_setter)
        iter_bf(s, reg, print_cleaner)
        iter_bf(s, reg, print_bit_mask)
        iter_bf(s, reg, print_bit_shift)
        if is_true_bitmask(s, reg):
            print_bitmask_printer(s, reg)
        print_printer(s, reg)

if __name__ == '__main__':
    specfile = sys.argv[1]
    main(specfile)

