Training data from https://database.lichess.org/#evals from 06/11/2024 with a total of 116,899,707 positions from
which 77,023,571 are quiet positions. 

To replicate the data, download the database from lichess, decompress and pass it to `json_to_csv.py`, it will
generate a csv with (FEN, Evaluation, Best move) for each position, then run `main.cpp` to filter out 
positions that are not quiet and store the inputs to the NNUE in `lichess_db_all_data.bin`. That is ready
to be used in training, but for convenience `data_to_batches.py` will separate the data in batches.
