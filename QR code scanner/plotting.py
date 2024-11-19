import pandas as pd
from datetime import datetime
import matplotlib.pyplot as plt

csv_files = ["24qr165hz160fps_discord.csv", "24qr165hz160fps.csv", 
             "30qr165hz160fps_discord.csv", "30qr165hz160fps.csv", 
             "60qr165hz160fps_discord.csv", "60qr165hz160fps.csv"]

def plot_latency_measurements(csv_file):
    # Load data
    data = pd.read_csv(csv_file, header=None, names=["timestamp1", "timestamp2", "frame"])

    # Convert frames to integers
    data['frame'] = data['frame'].str.extract('(\d+)').astype(int)

    # Calculate differences in milliseconds
    data['time_difference_ms'] = data.apply(lambda row: (datetime.strptime(row['timestamp1'], "%Y-%m-%d %H:%M:%S.%f") - 
                                                             datetime.strptime(row['timestamp2'], "%Y-%m-%d %H:%M:%S.%f")).total_seconds() * 1000, axis=1)

    # Discard min and max values from time differences
    #filtered_data = data[(data['time_difference_ms'] != data['time_difference_ms'].min()) & 
    #                     (data['time_difference_ms'] != data['time_difference_ms'].max())]

    # Calculate statistics on filtered data
    min_delay = data['time_difference_ms'].min()
    max_delay = data['time_difference_ms'].max()
    std_delay = data['time_difference_ms'].std()
    average_difference = data['time_difference_ms'].mean()

    # Calculate accuracy percentage
    total_frames = data['frame'].max() - data['frame'].min() + 1
    recorded_frames = data['frame'].nunique()
    accuracy_percentage = (recorded_frames / total_frames) * 100
    accuracy_label = f"Accuracy: {accuracy_percentage:.2f}%"

    summary_text = (f"Min Delay = {min_delay:.0f} ms, Max Delay = {max_delay:.0f} ms, Std = {std_delay:.0f} ms, Accuracy = {accuracy_percentage:.2f}%")

    plt.figure(figsize=(10, 6))
    plt.plot(data['frame'], data['time_difference_ms'], linestyle='-', color='b', linewidth=0.9, label='Time Difference (ms)')
    plt.axhline(y=average_difference, color='r', linestyle='--', label=f'Average: {average_difference:.2f} ms')

    # Add labels and annotations
    plt.title(summary_text, pad=20)
    plt.xlabel('Frames')
    plt.ylabel('Delay (ms)')
    plt.legend()
    plt.show()

plot_latency_measurements(csv_files[0])
plot_latency_measurements(csv_files[1])
plot_latency_measurements(csv_files[2])
plot_latency_measurements(csv_files[3])
plot_latency_measurements(csv_files[4])
plot_latency_measurements(csv_files[5])
