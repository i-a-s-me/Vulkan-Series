#!/bin/python3

import sys

def main(args: list[str]):
    if len(args) < 2:
        print(f"Usage: {args[0]} <input_file_path>")
        exit(1)

    with open(args[1], 'rb') as f:
        data = f.read()
        while (len(data) % 4) != 0: data += b'0'

        data_uint32 = []
        for i in range(0, len(data), 4):
            data_uint32.append((data[i + 3] << 24) | (data[i + 2] << 16) | (data[i + 1] << 8) | data[i + 0])

        print("static constexpr uint32_t SHADER_SPV_[] = {")

        ENTRIES_PER_LINE = 8
        count = len(data_uint32) // ENTRIES_PER_LINE
        remaining = len(data_uint32) % ENTRIES_PER_LINE

        for i in range(count):
            print('\t', end='')
            for j in range(ENTRIES_PER_LINE):
                print(f"0x{data_uint32[ENTRIES_PER_LINE*i + j]:08X},", end='')
            print('')
        
        print('\t', end='')
        for j in range(remaining):
            print(f"0x{data_uint32[ENTRIES_PER_LINE*count + j]:08X},", end='')
        print('')

        print("};")

if __name__ == "__main__":
    main(sys.argv)