# Top-down base port – notes

## CSV format (numbers only)
Resources/fields/*.csv : 100x100 cells, comma separated. 0 = blank, 1 = block, 2 = player spawn.
Resources/stageData.csv : `name,timeLimit,chunksX,chunksZ`  (last two optional, default 1, max 5)

## Chunks
- (0,0)  name.csv        base chunk, its bottom row is world z = 0, first column x = 0
- (x,0)  name_x1.csv     name_x2.csv ...   -> +X
- (0,z)  name_z1.csv     name_z2.csv ...   -> +Z
- (x,z)  name_x1_z1.csv                    -> generated only when BOTH (x,0) and (0,z) are occupied (file exists);
                                             if its own file is missing it is generated blank.
A missing chunk file never asserts; the chunk is just empty. The map size comes only from stageData.csv.

## World
Top-down: grid on the X-Z plane, screen-right = +X, screen-up = +Z, camera straight above (CameraController::SetHeight).
Player: WASD, Esc = back to title.
