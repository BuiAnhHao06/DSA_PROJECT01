## Thành viên 1 - Core Engine Lead

# 1. Các hằng số quan trọng

## PAGE_SIZE = 4096

- Mỗi page có kích thước 4KB = 4096 bytes.
- Mỗi node của B+ Tree là một page.
- Đọc ghi theo từng page = 4096 bytes.

## MAX_LEAF_KEYS = 60

- Một leaf node chứa tối đa 60 records.
- Khi số records hơn 60 thì tiến hành split leaf.

## MAX_INTERNAL_KEYS = 510

- Một internal node chứa tối đa 510 keys.
- Một internal node có tối đa 511 children.

# 2. Cấu trúc dữ liệu

## 2.1 Record

Cấu trúc:

- id : key, dùng để tìm kiếm
- payload : dữ liệu mô phỏng

Kích thước của Record = 64 bytes
Mỗi Leaf chứa tối đa 60 Record, 60 \* 64 = 3840. Không vượt quá giới hạn 1 page

## 2.2 BPlusNode

Mỗi node của cây được lưu dưới dạng một page trên đĩa.

Một node có thể là:

- Leaf Node
- Internal Node

### Leaf Node

Leaf node chứa dữ liệu thật.

Các thành phần:

- records[]
- next_leaf_offset : các leaf nối với nhau thông qua next_leaf_offset giúp hỗ trợ hỗ trợ range query

Ví dụ:

[10]
[20]
[30]
[40]

Leaf node là nơi chứa toàn bộ records của database.

### Internal Node

Internal node không chứa dữ liệu thật.

Chức năng:

- điều hướng tìm kiếm
- xác định node con cần đi xuống

Ví dụ:

```
      [50 | 100]

     /    |     \

  C0      C1     C2
```

Ý nghĩa:

C0 chứa các key < 50

C1 chứa các key từ 50 đến 99

C2 chứa các key >= 100

---

## 2.3 DBHeader

DBHeader nằm ở page đầu tiên của file.

Offset = 0

Vai trò:

- quản lý metadata
- xác định root node
- xác định tổng số node

# 3. Disk Layout

Toàn bộ cây được lưu trong file index.dat.

Quy tắc:

Offset của node thứ k: offset = PAGE_SIZE × k

# 4. Byte Offset

Không được sử dụng:

Node\* child

vì node không nằm trong RAM.

Phải sử dụng:

child_offset: cho biết Node đó nằm ở vị trí byte thứ mấy trong file

# 5. Nguyên tắc Insert của B+ Tree

Khi insert:

Bắt đầu từ root

↓

Đi xuống internal nodes

↓

Tới leaf phù hợp

↓

Chèn record

Nếu node đầy:

↓

Split

↓

Đẩy separator key lên parent

---

# 6. Leaf Split

khi num_keys > MAX_LEAF_KEYS

Ví dụ:

Leaf trước khi split:

10 20 30 40 50

Sau khi chèn:

10 20 30 35 40 50

Tiến hành split:

Leaf trái:

10 20 30

Leaf phải:

35 40 50

Separator Key:

35

Đưa lên parent.

Điểm đặc biệt:

35 vẫn nằm trong leaf phải.

---

# 7. Internal Split

Ví dụ:

10 20 30 40 50

Median:

30

Sau split:

Internal trái:

10 20

Internal phải:

40 50

Separator:

30

Đưa lên parent.

Khác với leaf split là 30 không nằm ở node con

# 8. Root Split và Update Leaf Linked List

## Root Split

Ví dụ:
Root ban đầu:

[10 20 30 40 50]

Tiến hành Split
Root mới:

      [30]

     /    \

Left Right

--> Khi Root Split xong phải cập nhật Header.root_offset

## Leaf Linked List Update

Ví dụ:
Ban đầu:

Leaf1 ---> Leaf2

Sau khi Leaf1 split:

Leaf1 ---> NewLeaf ---> Leaf2

--> Phải cập nhật lại next_leaf_offset

# 9. Các hàm cần cài đặt

readNode() : đọc một node từ file

---

writeNode() : ghi một node xuống file

---

allocateNode() : cấp phát page mới

---

resetIOCounters() : reset bộ đếm Disk I/O

---

insertRecord() : hàm insert bên ngoài

---

insertRecursive():

- insert từ root xuống leaf
- xử lý split lan ngược lên trên
