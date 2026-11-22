
i = 0
total = 0

while i < 20000000:
    total += (i * 3 + 7) % 1009
    i += 1

print(total)
