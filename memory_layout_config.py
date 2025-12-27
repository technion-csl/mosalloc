import sys
import warnings
import pandas as pd
from enum import Enum
from typing import NamedTuple

class PageSize(Enum):
    BASE_PAGE_4KB = 1<<12
    HUGE_PAGE_2MB = 2<<20
    HUGE_PAGE_1GB = 1<<20

class Size:
    
    class Units(Enum):
        KB_IN_BYTES = 1<<10
        MB_IN_BYTES = 1<<20
        GB_IN_BYTES = 1<<30
    
    def __init__(self, size):
        if isinstance(size, str):
            self.bytes = Size.size_string_to_bytes(size)
        elif str(size).isdigit():
            self.bytes = int(size)
        else:
            raise ValueError(f"Invalid size format: {size} ({type(size)})")
    
    @staticmethod
    def unit_size_in_bytes(unit_str: str):        
        units_string = unit_str.lower()
        if units_string == "gb":
            units = Size.Units.GB_IN_BYTES
        elif units_string == "mb":
            units = Size.Units.MB_IN_BYTES
        elif units_string == "kb":
            units = Size.Units.KB_IN_BYTES
        else:
            sys.exit("Error: invalid string for the size")
        
        return units

    @staticmethod
    def size_string_to_bytes(size_string: str):
        size = -1
        if size_string.isdigit():
            size = int(size_string)
        else:  # the string should contain units
            string_length = len(size_string)
            number = int(size_string[0: string_length - 2])
            units_string = size_string[string_length - 2: string_length]
            units = Size.unit_size_in_bytes(units_string)
            size = number * units.value
        
        return size
    
    def __int__(self) -> int:
        return self.bytes

class RegionsList:
    
    def __init__(self, regions_list_str: str):
        self.regions_list_str = regions_list_str
        # print(f'===> {type(self.regions_list_str)} <===')
        # print(f'===> {self.regions_list_str} <===')
        if type(self.regions_list_str) is list:
            regions_list = self.regions_list_str
        else:
            regions_list = RegionsList.list_from_str(self.regions_list_str)
        
        print(f'===> regions_list: "{regions_list}" <===')
        orig_items_list = RegionsList.list_from_ranges(regions_list)
        # remove duplicates
        self.items_list = list(set(orig_items_list))
        self.count = len(self.items_list)
        self.has_duplicates = len(orig_items_list) != len(self.items_list)
        # initialize regions_list after removing duplicates and sorting
        self.regions_list = RegionsList.list_to_ranges(self.items_list)
    
    def items_count(self):
        return self.count
    
    def get_region_absolute_values(self, item_size=1, offset=0):
        ranges_pairs = set()
        for item_str in self.regions_list:
            if '-' in item_str:
                start, end = map(int, item_str.split('-'))
                weighted_range_start = start * item_size + offset
                weighted_range_end = end * item_size + offset
                pair = (weighted_range_start, weighted_range_end)
                ranges_pairs.add(pair)
            else:
                item_val = int(item_str)
                weighted_range_start = item_val * item_size + offset
                weighted_range_end = (item_val + 1) * item_size + offset
                pair = (weighted_range_start, weighted_range_end)
                ranges_pairs.add(pair)
        return ranges_pairs
    
    @staticmethod
    def list_to_ranges(lst):
        if type(lst) is str:
            lst = RegionsList.list_from_str(lst)
        if len(lst) == 0:
            return []
        
        ranges = []
        start = lst[0]
        end = lst[0]
        for i in range(1, len(lst)):
            if lst[i] - lst[i - 1] == 1:
                end = lst[i]
            else:
                if start == end:
                    ranges.append(str(start))
                else:
                    ranges.append(f"{start}-{end}")
                start = lst[i]
                end = lst[i]
        if start == end:
            ranges.append(str(start))
        else:
            ranges.append(f"{start}-{end}")
        return ranges

    @staticmethod
    def list_from_ranges(ranges):
        lst = []
        for item in ranges:
            if type(ranges) is str:
                ranges = RegionsList.list_from_str(ranges)
            if '-' in item:
                start, end = map(int, item.split('-'))
                lst.extend(range(start, end + 1))
            else:
                print(f'==> {item} ({type(item)})')
                lst.append(int(item))
        return lst
    
    @staticmethod
    def list_from_str(list_str):
        if '[' in list_str and ']' in list_str:
            list_str = list_str.strip('[]')
        
        if list_str == '':
            return []
        
        lst = list_str.split(',')
        return lst
    
    def __iter__(self):
        return iter(self.items_list)

class MosallocPool:
    class PoolType(Enum):
        brk = 1
        mmap = 2
        file = 3

    class PoolMetadata(NamedTuple):
        pool_type: str
        pool_size: Size 
        ragions_list_2mb: RegionsList
        offset_2mb: Size
        ragions_list_1gb: RegionsList
        offset_1gb: Size

    def __init__(self, config_df: pd.DataFrame, pool_type: str):
        self.config_df = config_df
        self.pool_type_str = pool_type
        self.pool_type = MosallocPool.str_too_pool_type(self.pool_type_str)
        self.pool_row = self.config_df.query(f'pool_type == "{self.pool_type_str}"').iloc[0]
        self.metadata = self.parse_metadata()

    def parse_metadata(self) -> PoolMetadata:
        pool_type = self.pool_row['pool_type']
        pool_size = Size(self.pool_row['pool_size'])
        ragions_list_2mb = RegionsList(self.pool_row['ragions_list_2mb'])
        offset_2mb = Size(self.pool_row['offset_2mb'])
        ragions_list_1gb = RegionsList(self.pool_row['ragions_list_1gb'])
        offset_1gb = Size(self.pool_row['offset_1gb'])
        metadata = MosallocPool.PoolMetadata(pool_type=pool_type,
                                             pool_size=pool_size,
                                             ragions_list_2mb=ragions_list_2mb,
                                             offset_2mb=offset_2mb,
                                             ragions_list_1gb=ragions_list_1gb,
                                             offset_1gb=offset_1gb)
        return metadata
    
    @staticmethod
    def str_too_pool_type(pool_type_str: str):
        if pool_type_str.lower() == 'brk':
            return MosallocPool.PoolType.brk
        elif pool_type_str.lower() == 'mmap':
            return MosallocPool.PoolType.mmap
        elif pool_type_str.lower() == 'file':
            return MosallocPool.PoolType.file
        else:
            raise KeyError(f'invalid PoolType: {pool_type_str}')
    
    def get_hugepages_regions(self, page_size: PageSize) -> list:
        if page_size == PageSize.HUGE_PAGE_2MB:
            return self.get_2mb_regions()
        elif page_size == PageSize.HUGE_PAGE_1GB:
            return self.get_1gb_regions()
        else:
            raise ValueError(f'invalid PageSize: {page_size}')
        
    def get_2mb_regions(self) -> RegionsList:
        return self.metadata.ragions_list_2mb
    
    def get_2mb_pages_count(self) -> int:
        return self.metadata.ragions_list_2mb.items_count()
    
    def get_2mb_regions_offset(self) -> int:
        return int(self.metadata.offset_2mb)
    
    def get_1gb_regions(self) -> RegionsList:
        return self.metadata.ragions_list_1gb
    
    def get_1gb_pages_count(self) -> int:
        return self.metadata.ragions_list_1gb.items_count()
    
    def get_1gb_regions_offset(self) -> int:
        return int(self.metadata.offset_1gb)
    
    def get_pool_size(self) -> int:
        return int(self.metadata.pool_size)

class MemoryLayout:
    def __init__(self, config_file_csv: str):
        self.config_file_name = config_file_csv
        self.config_df = pd.read_csv(self.config_file_name)
        self.brk_pool = MosallocPool(self.config_df, 'brk')
        self.mmap_pool = MosallocPool(self.config_df, 'mmap')
        self.file_pool = MosallocPool(self.config_df, 'file')
    
    def get_total_hugepages_2mb(self) -> int:
        return self.brk_pool.get_2mb_pages_count() + self.mmap_pool.get_2mb_pages_count()
    
    def get_total_hugepages_1gb(self) -> int:
        return self.brk_pool.get_1gb_pages_count() + self.mmap_pool.get_1gb_pages_count()

    def get_legacy_pool_config_rows(self, pool: MosallocPool):
        df = pd.DataFrame(columns=['type', 'pageSize', 'startOffset', 'endOffset'])
        # add 2MB pages
        offset_2mb = pool.get_2mb_regions_offset()
        regions_2mb = pool.get_2mb_regions()
        for start, end in regions_2mb.get_region_absolute_values(PageSize.HUGE_PAGE_2MB.value, offset_2mb):
            range = pd.DataFrame([{'type': pool.pool_type.name, 
                                  'pageSize': PageSize.HUGE_PAGE_2MB.value, 
                                  'startOffset': start, 
                                  'endOffset': end}])
            df = pd.concat([df, range], ignore_index=True)
        # add 1GB pages
        offset_1gb = pool.get_1gb_regions_offset()
        regions_1gb = pool.get_1gb_regions()
        for start, end in regions_1gb.get_region_absolute_values(PageSize.HUGE_PAGE_1GB.value, offset_1gb):
            range = pd.DataFrame([{'type': pool.pool_type.name, 
                                  'pageSize': PageSize.HUGE_PAGE_1GB.value, 
                                  'startOffset': start, 
                                  'endOffset': end}])
            df = pd.concat([df, range], ignore_index=True)
        
        return df
            
    def build_legacy_configuration_layout(self) -> pd.DataFrame:
        df = pd.DataFrame(columns=['type', 'pageSize', 'startOffset', 'endOffset'])
        # add pools sizes
        brk_size_r = pd.DataFrame([{'type': 'brk', 'pageSize': -1, 'startOffset': 0, 'endOffset': self.brk_pool.get_pool_size()}])
        mmap_size_r = pd.DataFrame([{'type': 'mmap', 'pageSize': -1, 'startOffset': 0, 'endOffset': self.mmap_pool.get_pool_size()}])
        file_size_r = pd.DataFrame([{'type': 'file', 'pageSize': -1, 'startOffset': 0, 'endOffset': self.file_pool.get_pool_size()}])
        df = pd.concat([df, brk_size_r, mmap_size_r, file_size_r], ignore_index=True)
        
        # add pools hugepages
        brk_hugepages_df = self.get_legacy_pool_config_rows(self.brk_pool)
        if len(brk_hugepages_df) > 0:
            df = pd.concat([df, brk_hugepages_df], ignore_index=True)
        mmap_hugepages_df = self.get_legacy_pool_config_rows(self.mmap_pool)
        if len(mmap_hugepages_df) > 0:
            df = pd.concat([df, mmap_hugepages_df], ignore_index=True)
        
        return df

    @staticmethod
    def interleave_ranges(start1, end1, start2, end2):
        # region1 starts and ends before region2
        if start1 <= start2 and end1 <= start2:
            return False
        # region1 starts and ends after region2
        if start1 >= end2 and end1 >= end2:
            return False
        return True
            
    def validate_pool_configuration(self, pool: MosallocPool):
        offset_2mb = pool.get_2mb_regions_offset()
        regions_2mb = pool.get_2mb_regions()
        expanded_ranges_2mb = regions_2mb.get_region_absolute_values(PageSize.HUGE_PAGE_2MB.value, offset_2mb)
        
        offset_1gb = pool.get_1gb_regions_offset()
        regions_1gb = pool.get_1gb_regions()
        expanded_ranges_1gb = regions_1gb.get_region_absolute_values(PageSize.HUGE_PAGE_1GB.value, offset_1gb)
        
        # 1) validate regions are within pool size
        # 1.1) validate 2MB regions are within pool size
        if regions_1gb.items_count() > 0 and regions_2mb.items_count() > 0:
            range0_start_2mb, range0_end_2mb = expanded_ranges_2mb[0]
            rangeN_start_2mb, rangeN_end_2mb = expanded_ranges_2mb[-1]
            assert range0_start_2mb >= 0 and range0_end_2mb <= pool.get_pool_size(), f'first 2MB region of {pool.pool_type} pool is out of range: {range0_start_2mb}-{range0_end_2mb}'
            assert rangeN_start_2mb >= 0 and rangeN_end_2mb <= pool.get_pool_size(), f'last 2MB region of {pool.pool_type} pool is out of range: {rangeN_start_2mb}-{rangeN_end_2mb}'
            # 1.2) validate 1GB regions are within pool size
            range0_start_1gb, range0_end_1gb = expanded_ranges_1gb[0]
            rangeN_start_1gb, rangeN_end_1gb = expanded_ranges_1gb[-1]
            assert range0_start_1gb >= 0 and range0_end_1gb <= pool.get_pool_size(), f'first 2MB region of {pool.pool_type} pool is out of range: {range0_start_1gb}-{range0_end_1gb}'
            assert rangeN_start_1gb >= 0 and rangeN_end_1gb <= pool.get_pool_size(), f'last 2MB region of {pool.pool_type} pool is out of range: {rangeN_start_1gb}-{rangeN_end_1gb}'
        
        # 2) validate that 2MB and 1GB offsets are aligned
        assert MemoryLayout.is_aligned(pool.get_2mb_regions_offset(), PageSize.BASE_PAGE_4KB.value), \
            f"{pool.pool_type} pool's 2MB-region offset is not aligned with 4KB: {pool.get_2mb_regions_offset()}"
        assert MemoryLayout.is_aligned(pool.get_1gb_regions_offset(), PageSize.BASE_PAGE_4KB.value), \
            f"{pool.pool_type} pool's 1GB-region offset is not aligned with 4KB: {pool.get_1gb_regions_offset()}"
        if pool.get_1gb_regions_offset() > 0 and pool.get_2mb_regions_offset() > 0:
            offsets_delta = abs(pool.get_1gb_regions_offset() - pool.get_2mb_regions_offset())
            assert MemoryLayout.is_aligned(offsets_delta, PageSize.BASE_PAGE_1GB.value), \
                f"{pool.pool_type} pool's 1GB-region and 2MB-region offsets were set but un-aligned with 1GB. 2MB-region offset: {pool.get_2mb_regions_offset()}, 1GB-region offset: {pool.get_1gb_regions_offset()}"
            
        # 3) validate 2MB and 1GB regions do not interleave
        for start_2mb, end_2mb in expanded_ranges_2mb:
            for start_1gb, end_1gb in expanded_ranges_1gb:
                if MemoryLayout.interleave_ranges(start_2mb, end_2mb, start_1gb, end_1gb):
                    assert False, \
                        f'{pool.pool_type} pool has interleave hugepage ranges: 2MB[{start_2mb}-{end_2mb}] - 1GB[{start_1gb}-{end_1gb}]'
        
    def validate_pools(self):
        self.validate_pool_configuration(self.brk_pool)
        self.validate_pool_configuration(self.mmap_pool)
        # TODO: validate that file-mmap'ed pool has no hugepages
    
    @staticmethod
    def is_aligned(num, alignment):
        return (num % alignment) == 0
