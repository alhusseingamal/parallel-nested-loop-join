import csv
import random
import datetime

def generate_customers(filename, num_records):
    """Generate a randomized customers CSV file with integer data."""
    with open(filename, "w", newline="") as file:
        writer = csv.writer(file)
        # Write header
        writer.writerow(["customer_id", "col_1", "col_2", "age", "col_4", "col_5", "col_6"])
        # Write randomized records
        for i in range(1, num_records + 1):
            col_1 = random.randint(1000, 9999)
            col_2 = random.randint(1000, 9999)
            age = random.randint(18, 20)
            col_4 = random.randint(100000, 999999)
            col_5 = random.randint(100, 999)
            col_6 = random.randint(10000, 99999)
            writer.writerow([i, col_1, col_2, age, col_4, col_5, col_6])

def generate_orders(filename, num_records, num_customers):
    """Generate a randomized orders CSV file with integer data."""
    with open(filename, "w", newline="") as file:
        writer = csv.writer(file)
        # Write header
        writer.writerow(["order_id", "customer_id", "col_1", "quantity", "total_price"])
        # Write randomized records
        for i in range(1, num_records + 1):
            customer_id = random.randint(1, num_customers)
            col_1 = random.randint(1000, 9999)
            quantity = random.randint(1, 10)
            total_price = random.randint(100, 5000)
            writer.writerow([i, customer_id, col_1, quantity, total_price])

# Generate the randomized CSV files
customer_count = 21000
order_count = 50000
generate_customers("tc2_customers.csv", customer_count)
generate_orders("tc2_orders.csv", order_count, customer_count)

print("Randomized customer and order CSV files have been generated successfully!")
print(f"Customer Count: {customer_count}")
print(f"Order Count: {order_count}")