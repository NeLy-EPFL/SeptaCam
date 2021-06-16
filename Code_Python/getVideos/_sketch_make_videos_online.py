from subprocess import run
from time import sleep, time
from tempfile import mkstemp
from pathlib import Path
from multiprocessing import Pool


def _count_total_frames(cams_frames):
    """Return the total number of frames in a dictionary of the format
    {<cam_id>: {<frame_id>: <path>}}"""
    return sum([len(cams_frames[x]) for x in cams_frames])

def _run_compression(in_paths, video_path, metadata_path, fps, delete_images):
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
    f, filelist_path = mkstemp(suffix='.txt', text=True)
    lines = [f"file '{str(x.absolute())}'" for x in in_paths]
    f.write('\n'.join(lines))
    f.close()

    # Call FFmpeg
    run(['ffmpeg', '-r', fps, '-pix_fmt', pix_fmt, '-loglevel', 'warning',
         'concat', '-i', filelist_path, video_path], check=True)
    
    # Write matadata
    with open(metadata_path, 'w') as f:
        lines = [str(x.absolute()) for x in in_paths]
        f.write('\n'.join(lines))

    # Clean up
    if to_delete_images:
        for img_file in in_paths:
            img_file.unlink()
    Path(filelist_path).unlink()

def _run_compression_wrapper(config):
    return _run_compression(*config)

class Mp4Compressor:
    def __init__(self, fps, data_dir, cameras, delete_images=True,
                 pix_fmt='yuv420p', max_video_length_secs=300,
                 refresh_interval_secs=5, num_procs=4):
        self.fps = fps
        self.data_dir = data_dir
        self.cameras = cameras
        self.delete_images = delete_images
        self.max_video_length_secs = max_video_length_secs
        self.refresh_interval_secs = refresh_interval_secs
        self.max_frames_per_video = int(fps * max_video_length_secs)
        self.num_procs = num_procs
        self.pending_frames = {x: {}) for x in cameras}
        self.has_started = False
        self.part_count = 0
    
    def start(self):
        last_check_time = time()
        while True:
            if self.check():
                curr_time = time()
                sleep(max(0, last_check_time + self.refresh_interval_secs
                                             - time()))
            else:
                break
        print("All done")
    
    def check(self):
        """Check new files, and generate video part if necessary.

        Returns
        -------
        bool
            Whether or not this compressor needs to keep running.
            I.e. return False if and only if the compressor has
            detected at least one frame in the past, but has not
            detected new frames since the last check.
        """
        old_total_frames = _count_total_frames(self.pending_frames)
        self._add_new_files_to_pending_queue()
        new_total_frames = _count_total_frames(self.pending_frames)

        # Recording may take some time to initiate. Upon the first
        # frame being received, toggle has_started to True.
        if not self.has_started and new_total_frames:
            self.has_started = True
            return True
        
        # If every camera has >= max_frames_per_video pending frames,
        # take the first max_frames_per_video and convert to MP4.
        if all([len(x) >= self.max_video_length_secs]
                for x in self.pending_frames.values()):
            self._bind_video_multiprocess(wrap_up=False)
            return True    # keep running
        
        # If clock has already started, and no new frame's coming in,
        # we are done.
        if new_total_frames == old_total_frames:
            # Wrap up remaining frames and return False (all done)
            self._bind_video_multiprocess(wrap_up=True)
            return False    # all done
        
        return True    # keep running
    
    def _bind_video_multiprocess(self, wrap_up=False):
        job_configs = []
        for cam in self.cameras:
            if wrap_up:
                num_frames_to_compress = len(self.pending_frames[cam])
            else:
                self.max_frames_per_video
            frame_ids_sorted = sorted(self.pending_frames[cam].keys())
            frames_to_compress = frame_ids_sorted[:num_frames_to_compress]
            for frame in frames_to_compress:
                self.pending_frames[cam].pop(frame)
            in_paths = [self.pending_frames[cam][frame]
                        for frame in frames_to_compress]
            video_path = (self.data_dir.absolute().parent / 'videos'
                          / f'camera_{cam}_part_{self.part_count}.mp4')
            metadata_path = Path(
                str(video_path).replace('.mp4', '_framelist.txt'))
            job_configs.append([in_paths, video_path, metadata_path, self.fps,
                                self.delete_images])
        with Pool(self.num_procs) as pool:
            pool.map(_run_compression_wrapper, job_configs)
        
    
    def _add_new_files_to_pending_queue(self):
        for img_file in self.data_dir.glob('*.jpg'):
            info = img_file.name.replace.replace('.jpg', '').split('_')
            _, cam, _, frame = info
            if frame not in self.processed_frames[cam]:
                self.pending_frames[cam][frame] = img_file


if __name__ == '__main__':
    FIFO_PATH = '/tmp/mp4CompressorComm.fifo'
    