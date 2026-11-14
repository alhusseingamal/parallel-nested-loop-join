import csv

def sort_csv_by_customer_id(filename):
    """Sorts a CSV file in ascending order based on customer_id (as an integer)."""
    with open(filename, "r", newline="") as file:
        reader = csv.reader(file)
        header = next(reader)  # Read header
        sorted_rows = sorted(reader, key=lambda row: int(row[0]))  # Convert customer_id to int before sorting
    
    sorted_filename = f"sorted_{filename}"
    
    with open(sorted_filename, "w", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(header)  # Write header
        writer.writerows(sorted_rows)  # Write sorted rows

    print(f"Sorted file saved as {sorted_filename}")

# Example usage:
sort_csv_by_customer_id("gpu_out_nlj.csv")  # Sorts by customer_id as an integer
