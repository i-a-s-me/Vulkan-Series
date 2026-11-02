#!/bin/python3

import sys

def main(args: list[str]):
    if len(args) < 2:
        print(f"Usage: {args[0]} <input_file_path>")
        exit(1)

    with open(args[1], 'rb') as f:
        data = f.read()

        print("static constexpr uint8_t DATA[] = {")

        BYTES_PER_LINE = 16
        count = len(data) // BYTES_PER_LINE
        remaining = len(data) % BYTES_PER_LINE

        for i in range(count):
            print('\t', end='')
            for j in range(BYTES_PER_LINE):
                print(f"0x{data[BYTES_PER_LINE*i + j]:02X},", end='')
            print('')
        
        print('\t', end='')
        for j in range(remaining):
            print(f"0x{data[BYTES_PER_LINE*count + j]:02X},", end='')
        print('')

        print("};")

if __name__ == "__main__":
    main(sys.argv)