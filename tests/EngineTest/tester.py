import random
from subprocess import Popen, PIPE

win_eng1 = 0
win_eng2 = 0
draws = 0
canceled = 0
engine1_v = ".\Versions\V1"
engine2_v = ".\Versions\V1"
N = 100
print_moves = False

for _ in range(1, N+1):
    # Initialize engines
    engine1 = Popen([engine1_v, '-q', '-t', '100'], stdin=PIPE, stdout=PIPE)
    engine2 = Popen([engine2_v, '-q', '-t', '100'], stdin=PIPE, stdout=PIPE)

    engine1_color = random.choice(["0", "1"])
    engine2_color = "0" if engine1_color == "1" else "1"
    is_engine1_turn = (engine1_color == "0") # TODO

    engine1.stdin.write((engine2_color + '\n').encode())
    engine1.stdin.flush()
    engine1.stdout.readline()

    engine2.stdin.write((engine1_color + '\n').encode())
    engine2.stdin.flush()
    engine2.stdout.readline()
    
    try:
        while True:
            if is_engine1_turn:
                move = engine1.stdout.readline().decode().strip()
            else:
                move = engine2.stdout.readline().decode().strip()

            if len(move) not in [4, 5]:
                print(f"Game {_} outcome: " + move)
                if (move[:4] == "Draw"):
                    draws += 1
                    break
                
                if (move[:5] == "Black"):
                    if (engine1_color == '1'): win_eng1 += 1
                    else: win_eng2 += 1
                    break

                if (move[:5] == "White"):
                    if (engine1_color == '0'): win_eng1 += 1
                    else: win_eng2 += 1
                    break

            if (print_moves): print(move)

            if is_engine1_turn:
                fen = engine1.stdout.readline().decode()
                if (print_moves): print(fen, end="")
                a = (move + '\n')
                engine2.stdin.write(a.encode())
                engine2.stdin.flush()
                engine2.stdout.readline()
                engine2.stdout.readline()
            else:
                fen = engine2.stdout.readline().decode()
                if (print_moves): print(fen, end="")
                a = (move + '\n')
                engine1.stdin.write(a.encode())
                engine1.stdin.flush()
                engine1.stdout.readline()
                engine1.stdout.readline()

            is_engine1_turn = not is_engine1_turn
    except Exception:
        canceled += 1

    engine1.kill()
    engine2.kill()

print("Final score:")
print("Wins Engine 1:", win_eng1)
print("Wins Engine 2:", win_eng2)
print("Draws:", draws)
print("Games canceled:", canceled)
