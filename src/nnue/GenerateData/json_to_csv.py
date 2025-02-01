import csv
import json

with open('F:\\ChessDataSet\\Raw data\\lichess_db_eval.csv', 'w') as csv_f:
    writer = csv.writer(csv_f)
    writer.writerow(["FEN", "Evaluation", "Best move"])

    with open('F:\\ChessDataSet\\Raw data\\lichess_db_eval.jsonl') as f: 
        for line in f:
            position = json.loads(line)
    
            fen = position["fen"]
            eval = sorted(position["evals"], key=lambda x: x["depth"], reverse=True)[0]
            pv = eval["pvs"][0]
            best_move = pv["line"].split()[0]

            try:
                evaluation = pv["cp"]
            except KeyError:
                continue

            writer.writerow([fen, evaluation, best_move])
