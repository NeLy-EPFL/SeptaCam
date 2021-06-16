import os
from time import time, sleep
from queue import Queue
from pathlib import Path
from threading import Lock, Thread
from tempfile import mkstemp
from subprocess import run

def run_compression(in_paths, video_path, metadata_path, fps, delete_images):
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
    """
    # Build temporary file containing list of images
    # see https://trac.ffmpeg.org/wiki/Concatenate for specs
    f, filelist_path = mkstemp(suffix='.txt', text=True, dir='temp')    # TEST
    os.close(f)
    print(f'  >>> mkstemp: {filelist_path}')    # TEST
    lines = [f"file '{str(x.absolute())}'" for x in in_paths]
    with open(filelist_path, 'w') as f:
        f.write('\n'.join(lines))

    # Call FFmpeg
    # run(['ffmpeg', '-r', fps, '-pix_fmt', pix_fmt, '-loglevel', 'warning',
    #      'concat', '-i', filelist_path, video_path], check=True) # TEST
    sleep(1)    # TEST
    
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
    while True:
        cam, args = cmpr.job_queue.get()
        _guarded_print(
            cmpr.log_lock,
            f'>>> Thread {thread_id} working on {str(args[1].name)}'
        )
        cmpr.main_lock.acquire()
        cmpr.busy_workers += 1
        cmpr.main_lock.release()

        # Actual invocation
        start_time = time()
        run_compression(*args)
        end_time = time()

        # Increment part count
        cmpr.main_lock.acquire()
        cmpr.parts_done[cam] += 1
        cmpr.busy_workers -= 1
        cmpr.last_job_walltime = end_time - start_time
        cmpr.main_lock.release()

        cmpr.job_queue.task_done()

def _scan_files(cmpr):
    while True:
        cmpr.scan_lock.acquire()

        # Detect newly available files
        cmpr.add_new_files_to_pending_sets()

        # Move pending frames
        for cam in range(cmpr.num_cams):
            while len(cmpr.pending_frames[cam]) >= cmpr.frames_per_video:
                chunk = cmpr.pending_frames[cam][:cmpr.frames_per_video]
                cmpr.pending_frames[cam] = \
                    cmpr.pending_frames[cam][cmpr.frames_per_video:]
                cmpr.add_chunk_to_queue(cam, chunk)
        
        cmpr.scan_lock.release()
        sleep(cmpr.refresh_interval_secs)


class Mp4Compressor:
    def __init__(self, fps, data_dir, num_cams, delete_images=True,
                 pix_fmt='yuv420p', video_length_secs=300,
                 refresh_interval_secs=1, num_procs=4):
        # Initialize static properties
        self.fps = fps
        self.data_dir = data_dir
        self.num_cams = num_cams
        self.delete_images = delete_images
        self.video_length_secs = video_length_secs
        self.refresh_interval_secs = refresh_interval_secs
        self.frames_per_video = int(fps * video_length_secs)
        self.num_procs = num_procs

        # Initialize dynamic data structures and variables
        self.pending_frames = {cam: [] for cam in range(num_cams)}
        self.job_queue = Queue()
        self.parts_done = {cam: 0 for cam in range(num_cams)}
        self.parts_enqueued = {cam: 0 for cam in range(num_cams)}
        self.latest_frame_per_cam = {cam: -1 for cam in range(num_cams)}
        self.busy_workers = 0
        self.last_job_walltime = None

        # Initialize parallelization setup
        self.main_lock = Lock()
        self.log_lock = Lock()
        self.scan_lock = Lock()
        self.worker_threads = [
            Thread(daemon=True, target=_thread_worker, args=(i, self))
            for i in range(num_procs)
        ]
        self.scanner_thread = Thread(target=_scan_files, args=(self,),
                                     daemon=True)
    
    def start(self):
        for thread in self.worker_threads:
            thread.start()
        self.scanner_thread.start()

    def stop(self):
        # ASSUME any delay in IO (eg camera grab/save) would be within 1s
        sleep(1)

        # Add remaining frames to queue
        self.scan_lock.acquire()
        self.add_new_files_to_pending_sets()
        for cam in range(self.num_cams):
            chunk = self.pending_frames[cam]
            self.pending_frames[cam] = []
            self.add_chunk_to_queue(cam, chunk)
        self.scan_lock.release()

        # Now we just wait for everything to finish. This is blocking.
        self.job_queue.join()
        print('>>> All done!')
    
    def add_chunk_to_queue(self, cam, chunk):
        video_path = (self.data_dir / 'videos'
                      / f'camera_{cam}_part_{self.parts_enqueued[cam]}.mp4')
        self.parts_enqueued[cam] += 1
        metadata_path = Path(str(video_path).replace('.mp4', '.list'))
        args = (chunk, video_path, metadata_path, self.fps, self.delete_images)
        self.job_queue.put((cam, args))

    def add_new_files_to_pending_sets(self):
        # # What's the latest available image for each camera?
        # latest_frame_per_cam = {}
        # for cam in range(self.num_cams):
        #     if not self.pending_frames[cam]:
        #         latest_frame_per_cam[cam] = -1
        #     else:
        #         last_file = self.pending_frames[cam][-1].name
        #         frame = last_file.replace('.jpg', '').split('_')[-1]
        #         latest_frame_per_cam[cam] = int(frame)

        # Parse glob result, group by cam
        files_by_cam = {cam: [] for cam in range(self.num_cams)}
        for img_file in (self.data_dir / 'images').glob('*.jpg'):
            info = img_file.name.replace('.jpg', '').split('_')
            _, cam, _, frame = info
            cam = int(cam)
            frame = int(frame)
            if frame > self.latest_frame_per_cam[cam]:
                files_by_cam[cam].append((frame, img_file))

        # Register newly added files in order
        for cam in range(self.num_cams):
            if not files_by_cam[cam]:
                continue
            new_files = sorted(files_by_cam[cam], key=lambda x: x[0])
            self.pending_frames[cam] += [x[1] for x in new_files]
            self.latest_frame_per_cam[cam] = new_files[-1][0]

if __name__ == '__main__':
    FIFO_PATH = '/tmp/mp4CompressorComm.fifo'
    if os.path.exists(FIFO_PATH):
        os.unlink(FIFO_PATH)
    if not os.path.exists(FIFO_PATH):
        os.mkfifo(FIFO_PATH)
    
    compressors = {}

    fifo = open(FIFO_PATH, 'r')
    while True:
        request = fifo.readline().strip()
        if not request:
            continue
        print(f'>>> Got request: "{request}"')

        # Parse request
        cmd, fps, data_dir, num_cams = request.split(',')
        fps = float(fps)
        data_dir = Path(data_dir)
        num_cams = int(num_cams)

        if cmd == 'START':
            cmpr = Mp4Compressor(
                fps, data_dir, num_cams, num_procs=1,    # TEST
                delete_images=False, video_length_secs=10    # TEST
            )
            cmpr.start()
            compressors[data_dir] = cmpr
        elif cmd == 'STOP':
            compressors[data_dir].stop()
        else:
            raise ValueError(f'Unrecognized command "{cmd}" from FIFO file')