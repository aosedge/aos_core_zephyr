#!/usr/bin/python3
'''
Script gen_ta_src.py used for generating source files which creates lookup table
of loadable ta's. This table used by tee supplicant to load ta to OP-TEE
Options:
  -u [UUIDS ...], --uuids [UUIDS ...]
                        List of enabled TA's
  -f FILE, --file FILE  Config file with enabled TA's
  -o OUT, --out OUT     Output file name (without extension)
  -s SOURCE, --source SOURCE
                        Directory with prebuilt ta
  -t TABLE, --table TABLE
                        table variable name
  -p                    Print list of processed TA's
List ot ta can be passed to script follows ways:
  - with option -u can be pointed in command line
  - with option -f list of uuids can be enumerated in text file
  - with option -s script reads all files with extension .ta and used
                   it as list of enabled uuids

Script generates two files with extensions .c and .S and name pointed with -o option
To use lookup table in application genarated files just should be build with application.
'''
import os
import argparse
import uuid

def read_uuids_file(enabled_file):
    uuids = []
    for line in enabled_file:
        line = line.strip()
        if len(line) > 0 and line[0] != "#":
            if line not in uuids:
                uuids.append(uuid.UUID(line))
    return uuids

def generate_c(uuids, output, tablename):
    filename=output+'.c'
    file = open(filename, 'w')
    file.write('/* Do not modify. This is generated file*/\n')
    file.write('#include <zephyr/kernel.h>\n')
    file.write('#include <zephyr/init.h>\n')
    file.write('#include <teec_ta_load.h>\n\n')
    for u in uuids:
        uuid_str = str(u).replace('-', '_')
        file.write(f"extern unsigned char __ta_{uuid_str}_start[];\n")
        file.write(f"extern size_t __ta_{uuid_str}_size;\n")
    file.write(f'\nstruct ta_table {tablename}[] = ' + '{\n')
    for u in uuids:
        uuid_str = str(u).replace('-', '_')
        file.write("\t{\n\t\t" + f'.uuid = "{str(u)}",\n' + '\t},\n')
    file.write('\t{\n\t\t.uuid = NULL,\n\t}\n};\n\n')
    file.write('static int init_table(const struct device *dev)\n{\n')
    i = 0
    for u in uuids:
        uuid_str = str(u).replace('-', '_')
        file.write(f'\t{tablename}[{i}].ta_start = __ta_{uuid_str}_start;\n')
        file.write(f'\t{tablename}[{i}].ta_size = __ta_{uuid_str}_size;\n')
        i += 1
    file.write(f'\n\tTEEC_SetTATable({tablename});\n\treturn 0;\n')
    file.write("}\n\n")
    file.write('SYS_INIT(init_table, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);')
    file.close()

def genarate_asm(uuids, output):
    filename=output+'.S'
    file = open(filename, 'w')
    file.write('/* Do not modify. This is generated file*/\n')
    file.write('#include <zephyr/toolchain.h>\n\n')
    for u in uuids:
        uuid_str = str(u).replace('-', '_')
        file.write(f'.section .data.ta_{uuid_str},"a"\n')
        file.write(f'GDATA(__ta_{uuid_str}_start)\n')
        file.write(f'GDATA(__ta_{uuid_str}_size)\n')
        file.write(f'.align 8\n')
        file.write(f'__ta_{uuid_str}_start:\n')
        file.write(f'.incbin  "{str(u)}.ta"\n')
        file.write(f'__ta_{uuid_str}_size: .long . - __ta_{uuid_str}_start\n\n')
    file.close()

def main():
    parser = argparse.ArgumentParser(description="Generating source files which creates lookup"
                                     " table of loadable ta's. This table used by tee supplicant"
                                     " to load ta to OP-TEE.")
    parser.add_argument("-u", "--uuids", nargs="*", help="List of enabled layers")
    parser.add_argument("-f", "--file", action="store", help="Config file with enabled TA's")
    parser.add_argument("-o", "--out", action='store',
                        help="Output file name (without extension)", default="output")
    parser.add_argument("-s", "--source", action='store', help="Directory with prebuilt ta")
    parser.add_argument("-t", "--table", action='store',
                        help="table variable name", default="table")
    parser.add_argument("-p", "--print", action="store_true", help="Print list of processed TA's")
    args = parser.parse_args()

    if args.uuids is None and args.file is None and args.source is None:
        return sys.exit("Prebuilt TA's wasn't provided")

    uuids = []
    if args.uuids is not None:
        uuids = [uuid.UUID(u) for u in args.uuids]

    if args.file is not None:
            try:
                with open(args.file, "r", encoding="utf-8") as enabled_file:
                    uuids.extend(read_uuids_file(enabled_file))
            except FileNotFoundError:
                print(f"File {args.file} can't be opened")

    if args.source:
        listdir = os.listdir(args.source)
        for item in listdir:
            if item.endswith(".ta"):
                u = uuid.UUID(os.path.splitext(os.path.split(item)[1])[0])
                if u not in uuids:
                    uuids.append(u)
    else:
        uuids = None

    if uuids:
        genarate_asm(uuids, args.out)
        generate_c(uuids, args.out, args.table)
        if args.print:
            print("List of TA's")
            for u in uuids:
                print(str(u))

if __name__ == "__main__":
    main()
