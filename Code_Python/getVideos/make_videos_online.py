import os
from time import time, sleep
from queue import Queue
from pathlib import Path
from multiprocessing import Process
from threading import Lock, Thread
from tempfile import mkstemp
from subprocess import run
from datetime import datetime
from json import dumps
from bisect import insort

from monitor_gui import CompressionMonitor


def run_compression(in_paths, video_path, metadata_path, fps, delete_images,
                    pix_fmt):
    """Run MP4 compression by calling FFmpeg.

    Parameters
    ----------
    in_paths : list of pathlib.Path
        List of source images in the correct order.
    video_path : pathlib.Path
        Path to which the output video should be stored.
    metadata_path : pathlib.Path
        Path to which the newline-separated list of source image files
        should be stored.
    fps : float
        Frames per second to be written to the video file's metadata.
    delete_images : bool
        Whether or not source images should be deleted afterward.
    pix_fmt : str
        -pix_fmt flag used for the output file in the FFmpeg call.
    """
    # Build temporary file containing list of images
    # see https://trac.ffmpeg.org/wiki/Concatenate for specs
    f, filelist_path = mkstemp(suffix='.txt', text=True)
    os.close(f)
    print(f'  >>> running compression: {video_path.name}')
    lines = [f"file '{str(x.absolute())}'" for x in in_paths]
    with open(filelist_path, 'w') as f:
        f.write('\n'.join(lines))

    # Call FFmpeg
    run(['ffmpeg', '-r', str(fps), '-loglevel', 'error',
         '-f', 'concat', '-safe', '0', '-i', str(filelist_path),
         '-pix_fmt', pix_fmt, str(video_path)], check=True)
    
    # Write matadata
    with open(metadata_path, 'w') as f:
        lines = [str(x.absolute()) for x in in_paths]
        f.write('\n'.join(lines))

    # Clean up
    if delete_images:
        for img_file in in_paths:
            img_file.unlink()
    Path(filelist_path).unlink()

def _guarded_print(lock, *args, **kwargs):
    lock.acquire()
    print(*args, **kwargs)
    lock.release()

def _thread_worker(thread_id, cmpr):
    """Entry point for the worker threads"""
    while True:
        task_spec = cmpr.job_queue.get()
        if task_spec is None:
            break
        cam, args = task_spec
        _guarded_print(
            cmpr.log_lock,
            f'>>> Thread {thread_id} working on {str(args[1].name)}'
        )
        cmpr.status_lock.acquire()
        cmpr.busy_workers += 1
        cmpr.status_lock.release()

        # Actual invocation
        start_time = time()
        run_compression(*args)
        end_time = time()

        # Increment part count
        cmpr.status_lock.acquire()
        cmpr.parts_done[cam] += 1
        cmpr.busy_workers -= 1
        cmpr.last_job_walltime = end_time - start_time
        cmpr.status_lock.release()

        cmpr.job_queue.task_done()

def _scan_files(cmpr):
    """Entry point for the scanner thread"""
    while True:
        cmpr.status_lock.acquire()
        all_done = cmpr.all_done
        cmpr.status_lock.release()
        if all_done:
            break

        cmpr.scan_lock.acquire()
        # Detect newly available files
        cmpr.add_new_files_to_pending_sets()
        # Move pending frames
        for cam in range(cmpr.num_cams):
            while len(cmpr.pending_frames[cam]) >= cmpr.frames_per_video:
                chunk = cmpr.pending_frames[cam].pop_chunk(
                    cmpr.frames_per_video)
                cmpr.add_chunk_to_queue(cam, chunk)
        cmpr.scan_lock.release()
        sleep(cmpr.refresh_interval_secs)


def _log_status(cmpr):
    """Entry point for the logger thread"""
    while True:
        cmpr.status_lock.acquire()
        all_done = cmpr.all_done
        cmpr.status_lock.release()
        if all_done:
            with open(cmpr.log_path, 'a+') as f:
                f.write('ALL DONE\n')
            break
        
        curr_time = time()

        # get all stats
        cmpr.status_lock.acquire()
        status = {
            'timestamp': str(datetime.now()),
            'refresh_interval': cmpr.refresh_interval_secs,
            'num_cams': cmpr.num_cams,
            'fps': cmpr.fps,
            'nprocs': cmpr.num_procs,
            'nprocs_running': cmpr.busy_workers,
            'qsize': cmpr.job_queue.qsize(),
            'last_job_walltime': cmpr.last_job_walltime,
            'nvids_made': sum(cmpr.parts_done.values()),
            'frames_pending': [len(cmpr.pending_frames[cam])
                               for cam in range(cmpr.num_cams)]
        }
        cmpr.status_lock.release()

        # write to file
        with open(cmpr.log_path, 'a+') as f:
            f.write(dumps(status) + '\n')

        sleep(curr_time + cmpr.log_interval_secs - time())


class DanglingFrames:
    def __init__(self, camera, data_dir):
        self.camera = camera
        self.data_dir = data_dir
        self.frame_ids = []
        self.frame_ids_set = {}    # same as frame_ids, for easier search
        self.discard_frames_before = 0
    
    def __len__(self):
        return len(self.frame_ids_set)

    def scan(self):
        files = (self.data_dir / 'images').glob(f'camera_{self.camera}*.jpg')
        for img_path in files:
            frame = int(img_file.name.replace('.jpg', '').split('_')[-1])
            if frame < self.discard_frames_before:
                # arrived way too late, discard
                continue
            if frame not in self.frame_ids_set:
                insort(self.frame_ids, frame)
                self.frame_ids_set.add(frame)

    def pop_chunk(self, chunk_size, block_time_secs=1):
        if chunk_size == -1:
            chunk_size = len(self)
        if self.frame_ids[chunk_size] - self.frame_ids[0] > chunk_size:
            # there are frames that haven't arrived yet, wait a bit
            sleep(block_time_secs)
            self.scan()
        chunk_ids = self.frame_ids[:chunk_size]
        self.frame_ids = self.frame_ids[chunk_size:]
        self.frame_ids_set -= set(chunk_ids)
        self.discard_frames_before = chunk_ids[-1] + 1
        return [self.data_dir / f'images/camera_{self.camera}_img_{x}.jpg'
                for x in chunk_ids]


class Mp4Compressor:
    """Manager of the background MP4 compression of camera data. One
    ``Mp4Compressor`` object should be associated with each recording.
    The user specifies, among other misc information, the intended
    length of each video part. This manager, in turn, binds the JPEGs
    into an MP4 every time a sufficient number of frames is available.
    The videos can be generated in parallel, allowing the user to
    generate videos using multiple CPU cores when recording with many
    cameras at high frame rates. Each compressor also spawns a simple
    GUI monior.

    Example
    -------
    The external API of the compressor consists of the ``.start()``
    and ``.stop()`` methods.
    >>> compressor = Mp4Compressor(...)
    >>> compressor.start()    # this returns immediately
    >>> sleep(100)            # compression runs in background
    >>> compressor.stop()     # this blocks briefly
    
    Implementation
    --------------
    There are two key data structures in this class: ``pending_frames``
    and ``job_queue``. The "lifecycle" of data is as follows:
    - Calculate the intended number of frames per video part `N`
      (from the intended video part length and the FPS).
    - Once the monitor that a camera has written a frame to the file
      system, the frame is initially registered at ``pending_frames``.
    - The frame stays there until there are `N` frames from the camera
      in question in ``pending_frames``.
    - The earliest `N` frames are then removed from ``pending_frames``.
      A compression task (containing the list of input frames, output
      video path, etc) is specified for this batch of frames. The job
      spec is added to ``job_queue``.
    - Worker threads monitor the ``job_queue``, and execute the
      compression jobs that are in the queue.
    
    Each ``Mp4Compressor`` object manages M + 2 threads and up to M + 1
    processes (`M` being ``num_procs``):
    - Scanner thread (x1): Scans the file system periodically and add
      newly arrived frames to ``pending_frames``. If ``pending_frames``
      is long enough, make job specs and push the specs to
      ``job_queue``. Note that since this thread doesn't do any actual
      heavy-lifting, it should never block.
    - Worker threads (xM): Run the compression jobs posted on
      ``job_queue`` through systems calls to ``ffmpeg``, which are
      managed by ``subprocess.run``.
    - Logger thread (x1): Writes the current stats of the compressor
      to a log file, which can then be read from the GUI.
    - GUI process (x1): Displays the current status. It has to be a
      sepearate process becasue Tk doesn't support multithreading.
    - FFmpeg processes (x up to M): Spawned by worker threads.

    The log is a text file where each line is either:
    - a JSON string containing the status of the compressor (including
      a timestamp), OR
    - the literal string ``"ALL DONE"``, signaling termination.
    """
    def __init__(self, fps, data_dir, num_cams, log_path,
                 delete_images=True, pix_fmt='yuv420p', video_length_secs=300,
                 refresh_interval_secs=1, num_procs=4, log_interval_secs=1):
        # Initialize static properties
        self.fps = fps
        self.data_dir = data_dir
        self.num_cams = num_cams
        self.delete_images = delete_images
        self.pix_fmt = pix_fmt
        self.video_length_secs = video_length_secs
        self.refresh_interval_secs = refresh_interval_secs
        self.frames_per_video = int(fps * video_length_secs)
        self.num_procs = num_procs
        self.log_path = log_path
        self.log_interval_secs = log_interval_secs

        # Initialize dynamic data structures and variables
        self.pending_frames = [DanglingFrames(cam, data_dir)
                               for cam in range(num_cams)]
        self.job_queue = Queue()
        self.parts_done = [0] * num_cams
        self.parts_enqueued = [0] * num_cams
        self.latest_frame_per_cam = [-1] * num_cams
        self.busy_workers = 0
        self.last_job_walltime = None
        self.all_done = False

        # Initialize parallelization setup
        self.status_lock = Lock()
        self.log_lock = Lock()
        self.scan_lock = Lock()
        self.worker_threads = [
            Thread(target=_thread_worker, args=(i, self), daemon=True)
            for i in range(num_procs)
        ]
        self.scanner_thread = Thread(target=_scan_files, args=(self,),
                                     daemon=True)
        self.logger_thread = Thread(target=_log_status, args=(self,),
                                    daemon=True)
    
    def start(self):
        for thread in self.worker_threads:
            thread.start()
        self.scanner_thread.start()
        self.logger_thread.start()

    def stop(self):
        # ASSUME any delay in IO (eg camera grab/save) would be within 1s
        sleep(1)

        # Add remaining frames to queue
        self.scan_lock.acquire()
        self.add_new_files_to_pending_sets()
        for cam in range(self.num_cams):
            chunk = self.pending_frames[cam].pop_chunk(-1)
            if chunk:
                self.add_chunk_to_queue(cam, chunk)
        self.scan_lock.release()

        # Now we just wait for everything to finish. This is blocking.
        self.job_queue.join()
        self.status_lock.acquire()
        self.all_done = True
        self.status_lock.release()
        self.scanner_thread.join()
        for _ in range(self.num_procs):
            self.job_queue.put(None)
        for thread in self.worker_threads:
            thread.join()
        self.logger_thread.join()
        print('>>> All done!')
    
    def add_chunk_to_queue(self, cam, chunk):
        video_path = (self.data_dir / 'videos'
                      / f'camera_{cam}_part_{self.parts_enqueued[cam]}.mp4')
        self.parts_enqueued[cam] += 1
        metadata_path = Path(str(video_path).replace('.mp4', '.list'))
        args = (chunk, video_path, metadata_path, self.fps,
                self.delete_images, self.pix_fmt)
        self.job_queue.put((cam, args))

    def add_new_files_to_pending_sets(self):
        for frames in self.pending_frames:
            frames.scan()


def init_monitor_gui(log_path):
    monitor = CompressionMonitor(log_path)
    monitor.mainloop()


if __name__ == '__main__':
    """The intent to create/destroy ``Mp4Compressor`` objects is
    communicated through a named pipe (via a FIFO file) from the C++
    recording GUI. Each request takes one line (ends with \\n). The
    protocol is as follows:
    - ``"START,<data_dir>,<fps>,<num_cams>"``
      This literal string (where the base data directory, FPS, number
      of cameras are substituted accordingly) signals the initiation
      of a compressor.
    - ``"STOP,<data_dir>"``
      This signals the termination of a compressor.
    - ``"EXIT"``
      This signals the termination of the program.
    """

    FIFO_PATH = '/tmp/mp4CompressorComm.fifo'
    if os.path.exists(FIFO_PATH):
        os.unlink(FIFO_PATH)
    if not os.path.exists(FIFO_PATH):
        os.mkfifo(FIFO_PATH)
    
    compressors = {}
    monitor_processes = {}

    fifo = open(FIFO_PATH, 'r')
    while True:
        request = fifo.readline().strip()
        if not request:
            continue
        print(f'>>> Got request: "{request}"')

        cmd = request.split(',')[0]

        if cmd == 'START':
            # Parse command
            cmd, data_dir, fps, num_cams = request.split(',')
            fps = float(fps)
            data_dir = Path(data_dir)
            num_cams = int(num_cams)

            (data_dir / 'videos').mkdir(exist_ok=True)
            log_path = data_dir / 'compression_log.txt'
            compressors[str(data_dir)] = Mp4Compressor(
                fps, data_dir, num_cams, log_path,
                num_procs=7, delete_images=True, video_length_secs=10
            )
            compressors[str(data_dir)].start()
            monitor_processes[str(data_dir)] = Process(
                target=init_monitor_gui, args=(log_path,), daemon=True
            )
            monitor_processes[str(data_dir)].start()
        elif cmd == 'STOP':
            # Parse command
            cmd, data_dir = request.split(',')
            compressors[data_dir].stop()
            monitor_processes[data_dir].join()
        elif cmd == 'EXIT':
            break
        else:
            raise ValueError(f'Unrecognized command "{cmd}" from FIFO file')
