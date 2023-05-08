import sys
import os
import re
import numpy as np
import matplotlib.pyplot as plt
import cv2
import subprocess


def main(input_file, cols, iterations):
    # Parse input file name
    tmp = re.findall('(\d+)', input_file)
    if cols == 0:
        if len(tmp) == 3:
            rows = int(tmp[0])
            cols = int(tmp[1])
            iterations = int(tmp[2])+1
            print(f"Processing '{input_file}' as {iterations} iterations of matrix size {rows}x{cols}")
        else:
            print(f"Error: <all stacked file> name must include rows, cols, and iterations in order, delimited with any non-numeric chars. \ne.g., 'all100x100x10.raw', '100-100-50_stacked.raw', '100.100.50_new', etc.")
            exit(1)
    else:
        rows = int(input_file)
        gcc_cmd = ['./pth-stencil-2d', str(iterations), f'mat-{str(rows)}-{str(cols)}.dat', 'mat_out.dat' , '0', str(2), f'{str(rows)}-{str(cols)}-{str(iterations)}.raw']
        make_data = subprocess.run(['make', 'clean', 'all'], stdout=subprocess.PIPE)
        make_data = subprocess.run(['./make-2d', str(rows), str(cols), f'mat-{str(rows)}-{str(cols)}.dat'], stdout=subprocess.PIPE)
        make_data = subprocess.run(gcc_cmd, stdout=subprocess.PIPE)
        input_file = f'{str(rows)}-{str(cols)}-{str(iterations)}.raw'
        iterations+=1
    # Read binary data and reshapes into a 3D array
    data = np.fromfile(input_file, dtype=np.double)
    expected_size = rows * cols * iterations
    actual_size = data.size

    if expected_size != actual_size:
        print(f"Error: Expected data size {expected_size} does not match actual size {actual_size}")
        sys.exit(1)
    data = data.reshape((iterations, rows, cols))

    # Create a color map
    cmap = plt.get_cmap("coolwarm")

    #creates video folder if one does not exist
    vid_dir = "./videos/"
    if not os.path.exists(vid_dir):
        os.makedirs(vid_dir)

    # Set up video writer
    video_file = f"heatmap-{rows}x{cols}x{iterations}.mp4"
    video_path = f"{vid_dir}{video_file}"
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    video_writer = cv2.VideoWriter(video_path, fourcc, 90, (cols, rows))

    # Process each matrix and save as a video frame
    for matrix in data:
        # Normalize and map to color
        matrix_normalized = (matrix - matrix.min()) / (matrix.max() - matrix.min())
        matrix_color = (cmap(matrix_normalized)[:, :, :3] * 255).astype(np.uint8)

        # Convert to BGR format and write frame to video
        frame = cv2.cvtColor(matrix_color, cv2.COLOR_RGB2BGR)
        video_writer.write(frame)

    video_writer.release()
    print(f"Saved new mp4 video to '{video_path}'")


if __name__ == "__main__":
    if len(sys.argv) == 2:
        if not os.path.isfile(sys.argv[1], 0, 0):
            print(f"Error: file {sys.argv[1]} does not exist")
        else:
            main(sys.argv[1])
    elif len(sys.argv) == 4:
            main(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]))
    else:
        print(f"Usage: python3 {sys.argv[0]} <all stacked filename>")
        print(f"(Alt) Usage: python3 {sys.argv[0]} <rows> <columns> <iterations>")
