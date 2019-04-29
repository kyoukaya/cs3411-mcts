#!/usr/bin/python3
import asyncio
import time
import tqdm
import json
import csv
from math import *

base_port = 12340
port_mod = 0
lookt_depth = 16
num_workers = 5

c_vals = (0.4, 1, 1.3, 2)
c_vals = list(map(lambda x: sqrt(x), c_vals))


def create_queue_from_json(fp):
    total_jobs = 0
    entries = json.load(fp)
    queue = asyncio.Queue()
    print(total_jobs)
    for entry in entries:
        queue.put_nowait(entry)
        total_jobs += 1
    return queue, total_jobs


async def game(port, lookt_depth, first, exp_c, pbar, start_moves):
    servt = f"./servt -p {port} -m {start_moves[0]} {start_moves[1]}"
    lookt = f"./lookt -p {port} -d {lookt_depth}"
    agent = f"./agent -p {port}"

    serv_p = await asyncio.create_subprocess_shell(servt,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)
    await asyncio.sleep(0.2)

    if (first):
        proc = await asyncio.create_subprocess_shell(
            agent,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)
    else:
        await asyncio.create_subprocess_shell(lookt,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)

    await asyncio.sleep(0.2)

    if (first):
        await asyncio.create_subprocess_shell(lookt,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)
    else:
        proc = await asyncio.create_subprocess_shell(
            agent,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)

    servstdout, _ = await serv_p.communicate()
    stdout, stderr = await proc.communicate()

    if (stderr):
        pbar.update(stderr.decode())

    return stdout.decode(), servstdout.decode()


async def worker(queue, workerNo, pbar):
    port = base_port + workerNo * 10
    numDone = 0
    i = 0
    while True:
        line = await queue.get()

        # make worker 0 dump queue ever 10 games
        if (workerNo == 0 and i == 0):
            json.dump(list(queue._queue), open('queue', 'w'))

        res, servres = await game(port + i, lookt_depth, line['first'], 0, pbar,
                         start_moves=line['first_move'])
        pbar.write(servres[1:-1])
        numDone += 1
        with open("res.csv", "a") as fout:
            fout.write(res)
        await asyncio.sleep(0.1)
        pbar.update(1)
        queue.task_done()
        i += 1
        i = i % 10


async def main():
    total_jobs = 0
    # with open('tasks', 'r') as fin:
    #     total_jobs = 0
    #     queue, total_jobs = create_queue_from_json(fin)
    #     print(queue)

    queue = asyncio.Queue()
    # Fill the queue
    # Do 5 games of each kind for consistency
    for _ in range(3):
        # Empirical testing of C vals
        # for c_val in c_vals:
        # for the 3 symmetric boards 1,2,5
        for board in range(1, 10):
            # Alternate between First and Second
            for first in (True, False):
                # For each subboard iterate through 1..9
                for square in range(1, 10):
                    item = {
                        "first_move": (board, square),
                        "first": first,
                        # "exp_c": c_val
                    }
                    total_jobs += 1
                    queue.put_nowait(item)

    pbar = tqdm.tqdm(desc="MCTS test", total=total_jobs)

    tasks = []

    for i in range(num_workers):
        task = asyncio.create_task(worker(queue, i, pbar))
        tasks.append(task)

    started_at = time.monotonic()

    await queue.join()
    total_time = time.monotonic() - started_at
    for task in tasks:
        task.cancel()
    print(f"total_time {total_time} total jobs {total_jobs}")

asyncio.run(main())
