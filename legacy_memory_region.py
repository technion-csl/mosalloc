import sys
from typing import List

import pandas as pd
from collections import namedtuple

IntervalData = namedtuple('IntervalData', ['type', 'page_size', 'start_offset', 'end_offset'])

KB2B = 1024
MB2B = 1024 * KB2B
GB2B = 1024 * MB2B
BASE_PAGE_SIZE = 4 * KB2B
LARGE_PAGE_SIZE = 2 * MB2B
HUGE_PAGE_SIZE = GB2B


def is_aligned(num, alignment):
    return (num % alignment) == 0


def convert_size_string_to_bytes(size_string: str):
    if size_string.isdigit():
        size = int(size_string)
    else:  # the string should contain units
        string_length = len(size_string)
        number = int(size_string[0: string_length - 2])
        units_string = size_string[string_length - 2: string_length].lower()
        if units_string == "gb":
            units = GB2B
        elif units_string == "mb":
            units = MB2B
        elif units_string == "kb":
            units = KB2B
        else:
            sys.exit("Error: invalid string for the size")
        size = number * units
    return size


def validate_region_offsets(start_offset, end_offset, page_size):
    region_size = end_offset - start_offset
    # validate that end >= start
    if region_size < 0:
        sys.exit("Error: end_offset < start_offset")
    # validate that the region is aligned
    if (not is_aligned(start_offset, BASE_PAGE_SIZE)) or (not is_aligned(region_size, page_size)):
        sys.exit("Error: the region is not aligned")


class MemoryRegion:

    def __init__(self, csv_config_file: str, region_type: str):
        self.type = region_type
        self.csv_config_file = csv_config_file
        self.region_intervals = sorted(self.get_list_interval_of_region(csv_config_file, region_type),
                                       key=lambda x: x.start_offset)
        self.pool_size = self.validate_region_and_get_pool_size()

    @staticmethod
    def get_list_interval_of_region(csv_config_file: str, region_type: str) -> List[IntervalData]:
        config_data = pd.read_csv(csv_config_file)
        return [IntervalData(row[1].type, row[1].pageSize, row[1].startOffset, row[1].endOffset)
                for row in config_data.iterrows() if row[1].type == region_type]

    def get_num_of_large_pages(self):
        return self.get_num_of_page_size(LARGE_PAGE_SIZE)

    def get_num_of_huge_pages(self):
        return self.get_num_of_page_size(HUGE_PAGE_SIZE)

    def get_num_of_page_size(self, page_size):
        return sum([int((interval.end_offset - interval.start_offset) / page_size)
                    for interval in self.region_intervals if interval.page_size == page_size])

    def validate_region_and_get_pool_size(self):
        pool_size = -1
        for region in self.region_intervals:
            if region.page_size == -1:
                pool_size = region.end_offset
                self.region_intervals.remove(region)
                break
        if pool_size < 0:
            sys.exit("Error: regions configuration must include all pools size")

        region_2mb_list = [interval for interval in self.region_intervals if
                           interval.page_size == LARGE_PAGE_SIZE]

        region_1gb_list = [interval for interval in self.region_intervals if
                           interval.page_size == HUGE_PAGE_SIZE]

        # validate regions not exceed pool size
        for region_1gb, region_2mb in zip(region_1gb_list, region_2mb_list):
            if region_1gb.end_offset > pool_size or region_2mb.end_offset > pool_size:
                sys.exit("Error: regions exceed the pool size")

        for regions_page_size, regions_list in zip(["2mb regions", "1gb regions"], [region_2mb_list, region_1gb_list]):
            for index, region in enumerate(regions_list):
                validate_region_offsets(region.start_offset, region.end_offset, region.page_size)
                if index == 0:
                    continue
                if not is_aligned((region.start_offset - regions_list[index - 1].start_offset), region.page_size):
                    sys.exit("Error: invalid delta between {} offsets".format(regions_page_size))

        if len(region_2mb_list) != 0 and len(region_1gb_list) != 0:
            delta = abs(region_1gb_list[0].start_offset - region_2mb_list[0].start_offset)
            if not is_aligned(delta, LARGE_PAGE_SIZE):
                sys.exit("Error: invalid delta between 1gb and 2mb offsets")

        for index, region in enumerate(self.region_intervals):
            if index == len(self.region_intervals) - 1:
                break
            if region.page_size == -1:
                continue
            if region.end_offset > self.region_intervals[index+1].start_offset:
                sys.exit("Error: regions are overlapping")
        return pool_size
