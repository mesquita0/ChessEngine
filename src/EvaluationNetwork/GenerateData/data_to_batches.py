import os
import numpy as np

NUM_FILES = 50

path_all_data_file = "F:\\ChessDataSet\\lichess_db_all_data.bin"
path_data_directory = "F:\\ChessDataSet\\Data\\"

dt = np.dtype([
    ('indices_side_move', 'i4', (30,)),
    ('indices_side_not_move', 'i4', (30,)),
    ('evaluation', 'i4')
])
num_positions = os.path.getsize(path_all_data_file) / dt.itemsize

with open(path_all_data_file, mode='rb') as f:

    for i in range(NUM_FILES):
        path = path_data_directory + f"data_batch_{i}.bin"
        with open(path, 'wb'): pass # Erase previous contents of the file
        with open(path, 'ab') as out_f:
            data = np.fromfile(f, dtype=dt, count=int(num_positions/NUM_FILES))
            data.tofile(out_f)
