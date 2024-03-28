import argparse
import parser
import printer

def main(args):
  with open(args.filename, "r") as f:
    lines = [line for line in f]
    objs = parser.parse(lines)
    printer.print_files(objs, args.header_path, args.cpp_path)
    

if __name__ == '__main__':
  argparser = argparse.ArgumentParser()
  argparser.add_argument('-f', '--file', dest="filename", required=True, metavar="FILE")
  argparser.add_argument('-p', '--hpp', dest="header_path", required=True, metavar="PATH")
  argparser.add_argument('-c', '--cpp', dest="cpp_path", required=True, metavar="PATH")
  args = argparser.parse_args()
  main(args)
