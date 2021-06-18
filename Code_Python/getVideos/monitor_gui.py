import os
import tkinter as tk
from tkinter import messagebox
from json import loads


def _read_last_line(filename):
    """Efficiently read the last line from a file
    Modified from https://stackoverflow.com/a/54278929 (CC BY-SA 4.0)
    24 hours of log at 1 line per sec takes ~1e-3 sec to read
    """
    if not os.path.exists(filename):
        return None
    try:
        with open(filename, 'rb') as f:
            f.seek(-2, os.SEEK_END)
            while f.read(1) != b'\n':
                f.seek(-2, os.SEEK_CUR)
            last_line = f.readline().decode()
        return last_line
    except OSError:
        return None

def _set_label_value(widget, new_val):
    heading = widget['text'].split('    ')[0]
    widget['text'] = f'{heading}    {new_val}'


class CompressionMonitor(tk.Tk):
    def __init__(self, log_path):
        super().__init__()
        self.log_path = log_path
        self._init_view()
        self._update_view_event()
    
    def _update_view_event(self):
        last_log_line = _read_last_line(self.log_path)
        if last_log_line is None:
            pass
        elif last_log_line.strip() == 'ALL DONE':
            messagebox.showinfo('All done', 'Compression done')
            self.destroy()
        else:
            status = loads(last_log_line)
            self.update_view(status)
        self.after(1000, self._update_view_event)

    def _init_view(self):
        self.title('Compression Monitor')

        self.w_main_frame = tk.Frame(self)
        self.w_main_frame.pack(padx=10, pady=5, fill=tk.X, expand=10)

        # Recording info
        self.w_rec_info = tk.LabelFrame(
            self.w_main_frame,
            text='Recording Info'
        )
        self.w_refresh_interval = tk.Label(
            self.w_rec_info,
            text='Refresh interval:    None',
            width=30,
            anchor=tk.W
        )
        self.w_ncams = tk.Label(
            self.w_rec_info,
            text='Num cameras:    None'
        )
        self.w_fps = tk.Label(
            self.w_rec_info,
            text='Frames per sec:    None'
        )
        self.w_refresh_interval.pack(anchor=tk.NW)
        self.w_ncams.pack(anchor=tk.NW)
        self.w_fps.pack(anchor=tk.NW)
        self.w_rec_info.pack(pady=5, fill=tk.X)

        # Queue stats
        self.w_queue_info = tk.LabelFrame(
            self.w_main_frame,
            text='Queue Info',
        )
        self.w_nprocs = tk.Label(
            self.w_queue_info,
            text='Num processes:    None'
        )
        self.w_nprocs_running = tk.Label(
            self.w_queue_info,
            text='Num procs running:    None'
        )
        self.w_qsize = tk.Label(
            self.w_queue_info,
            text='Num videos pending:    None'
        )
        self.w_last_job_time = tk.Label(
            self.w_queue_info,
            text='Last job walltime:    None'
        )
        self.w_nvids_made = tk.Label(
            self.w_queue_info,
            text='Num videos made:    None'
        )
        self.w_nprocs.pack(anchor=tk.NW)
        self.w_nprocs_running.pack(anchor=tk.NW)
        self.w_qsize.pack(anchor=tk.NW)
        self.w_last_job_time.pack(anchor=tk.NW)
        self.w_nvids_made.pack(anchor=tk.NW)
        self.w_queue_info.pack(pady=5, fill=tk.X)

        # Overall progress
        self.w_pending_frames_info = tk.LabelFrame(
            self.w_main_frame,
            text='Dangling Frames',
        )
        self.wlist_nframes_pending = [tk.Label(
            self.w_pending_frames_info,
            text=f'Frames pending #{i}:    None'
        ) for i in range(7)]
        for w in self.wlist_nframes_pending:
            w.pack(anchor=tk.NW)
        self.w_pending_frames_info.pack(pady=5, fill=tk.X)

    def update_view(self, status):
        _set_label_value(self.w_refresh_interval,
                         f'{status["refresh_interval"]} sec')
        _set_label_value(self.w_ncams, status["num_cams"])
        _set_label_value(self.w_fps, status["fps"])

        _set_label_value(self.w_nprocs, status["nprocs"])
        _set_label_value(self.w_nprocs_running, status["nprocs_running"])
        _set_label_value(self.w_qsize, status["qsize"])
        if status["last_job_walltime"]:
            last_job_walltime_str = '{:.2f} sec'.format(status["last_job_walltime"])
        else:
            last_job_walltime_str = 'None'
        _set_label_value(self.w_last_job_time, last_job_walltime_str)
        _set_label_value(self.w_nvids_made, status["nvids_made"])
        
        for cam in range(status["num_cams"]):
            _set_label_value(self.wlist_nframes_pending[cam],
                             status['frames_pending'][cam])


def _test():
    test_log_path = '/Volumes/LaCie/SampleData/201130_aDN2-CsChr/Fly0/005_beh/behData/compression_log.txt'
    app = CompressionMonitor(test_log_path)
    app.mainloop()


if __name__ == '__main__':
    test()