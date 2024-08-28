import asyncio
import chess
import chess.pgn
import platform
import random

async def main():
    win_eng1 = 0
    win_eng2 = 0
    draws = 0
    canceled = 0
    engine1_v = ".\Versions\V6"
    engine2_v = ".\Versions\V6"
    engine1_time = 100
    engine2_time = 100
    N = 500
    NUM_RANDOM_MOVES_EACH = 2
    initial_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    print_moves = False
    save_games = True

    # Check fen
    try:
        board = chess.Board(fen=initial_fen)
    except ValueError:
        exit("Invalid Fen.")

    for i in range(1, N+1):
        game = chess.pgn.Game()
        
        # Begin each game with a set number of random moves for each engine
        # so that different games can have different outcomes.
        board = chess.Board(fen=initial_fen)
        for _ in range(NUM_RANDOM_MOVES_EACH*2):
            move = random.choice(list(board.legal_moves))
            if (_ == 0): node = game.add_main_variation(move)
            else: node = node.add_variation(move)
            board.push_uci(move.uci())

            if (print_moves):
                print(move.uci(), "(random)")
                print(board.fen())

        # Initialize engines
        engine1 = await asyncio.create_subprocess_exec(
            engine1_v, '-q', '-t', str(engine1_time), '--fen', *board.fen().split(),
            stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE)
        engine2 = await asyncio.create_subprocess_exec(
            engine2_v, '-q', '-t', str(engine2_time), '--fen', *board.fen().split(),
            stdin=asyncio.subprocess.PIPE, stdout=asyncio.subprocess.PIPE)

        # Set engine colors
        engine1_color = random.choice(["w", "b"])
        engine2_color = "w" if engine1_color == "b" else "b"
        turn = "w" if (board.turn == chess.WHITE) else "b"
        is_engine1_turn = (engine1_color == turn)

        game.headers["White"] = "Engine 1" if (engine1_color == "w") else "Engine 2"
        game.headers["Black"] = "Engine 1" if (engine1_color == "b") else "Engine 2"
        
        engine1.stdin.write((engine2_color + '\n').encode())
        await engine1.stdout.readline()

        engine2.stdin.write((engine1_color + '\n').encode())
        await engine2.stdout.readline()

        while True:
            if is_engine1_turn:
                 try:
                    async with asyncio.timeout((engine1_time + 100)/1000):
                        move = (await engine1.stdout.readline()).decode().strip()
                 except asyncio.TimeoutError:
                    move = ""
            else:
                try:
                    async with asyncio.timeout((engine1_time + 100)/1000):
                        move = (await engine2.stdout.readline()).decode().strip()
                except asyncio.TimeoutError:
                    move = ""
            
            if len(move) not in [4, 5]:
                print(f"Game {i} outcome: " + move)
                if (move[:4] == "Draw"):
                    draws += 1
                    break
                
                if (move[:5] == "Black"):
                    if (engine1_color == 'b'): win_eng1 += 1
                    else: win_eng2 += 1
                    break

                if (move[:5] == "White"):
                    if (engine1_color == 'w'): win_eng1 += 1
                    else: win_eng2 += 1
                    break
                
                if (move == ""): print("Canceled.")

                canceled += 1
                break

            if (print_moves): print(move)

            if is_engine1_turn:
                fen = (await engine1.stdout.readline()).decode()
                if (print_moves): print(fen, end="")

                engine2.stdin.write((move + '\n').encode())
                try:
                    node = node.add_variation(chess.Move.from_uci(move))
                except chess.InvalidMoveError:
                    canceled += 1
                    break

                await engine2.stdout.readline()
                await engine2.stdout.readline()
            else:
                fen = (await engine2.stdout.readline()).decode()
                if (print_moves): print(fen, end="")

                engine1.stdin.write((move + '\n').encode())
                try:
                    node = node.add_variation(chess.Move.from_uci(move))
                except chess.InvalidMoveError:
                    canceled += 1
                    break

                await engine1.stdout.readline()
                await engine1.stdout.readline()

            is_engine1_turn = not is_engine1_turn
        
        if engine1.returncode is None: 
            engine1.kill()
            engine1._transport.close()
        if engine2.returncode is None: 
            engine2.kill()
            engine2._transport.close()

        # Save games in PGN
        if (save_games and move != ''):
            with open(f".\\Games\\Game{i}.txt", 'w') as f:
                f.write(game.__str__())
            print(f"Game {i} saved at Games\\Game{i}.txt")

    print("Final score:")
    print("Wins Engine 1:", win_eng1)
    print("Wins Engine 2:", win_eng2)
    print("Draws:", draws)
    print("Games canceled:", canceled)


if platform.system() == 'Windows':
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

loop = asyncio.ProactorEventLoop()
asyncio.set_event_loop(loop)
loop.run_until_complete(main())
loop.close()
