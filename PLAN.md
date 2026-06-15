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

---

# 10. Các hàm Query và Benchmark cần cài đặt

## 10.1 Internal Search trong một node

Cần cài đặt 2 cách tìm vị trí key trong mảng đã được sắp xếp:

linearSearch(keys, num_keys, target)

- Duyệt lần lượt từ trái sang phải.
- Dễ cài đặt.
- Chi phí CPU là O(m), với m là số key trong node.

binarySearch(keys, num_keys, target)

- Chia đôi phạm vi tìm kiếm.
- Phù hợp vì key trong node luôn được sắp xếp tăng dần.
- Chi phí CPU là O(log m).

Hai hàm này không làm thay đổi số lần đọc đĩa, vì chúng chỉ xử lý dữ liệu đã được đọc vào RAM.

---

## 10.2 Point Query

Mục tiêu:

Tìm một record có id = target_id.

Quy trình:

1. Đọc root node từ header.root_offset
2. Nếu là internal node:
   - dùng linear search hoặc binary search để chọn child_offset phù hợp
   - đọc node con từ file
3. Lặp lại cho tới khi gặp leaf node
4. Tìm target_id trong records[] của leaf
5. Nếu tìm thấy thì trả về true, ngược lại trả về false

Point Query B+ Tree và B-Tree Style gần giống nhau vì đều phải đi từ root xuống leaf.

Kết quả quan trọng cần đo:

- thời gian chạy trung bình
- số lần đọc page từ đĩa

---

## 10.3 Range Query

Mục tiêu:

Đếm số record có id nằm trong đoạn [start_id, end_id].

### B+ Tree Style Range Query

Quy trình:

1. Đi từ root xuống leaf đầu tiên có thể chứa start_id
2. Quét records[] trong leaf hiện tại
3. Nếu chưa vượt quá end_id thì đi sang leaf kế tiếp bằng next_leaf_offset
4. Dừng khi:
   - key hiện tại > end_id
   - hoặc next_leaf_offset = -1

Ưu điểm:

- Không cần quay lại internal node nhiều lần.
- Các leaf được nối thành linked list.
- Range query chỉ cần đọc các leaf liên tiếp theo thứ tự logic.

### B-Tree Style Range Query

Quy trình:

Với từng id trong đoạn [start_id, end_id], thực hiện lại thao tác point query từ root xuống leaf.

Nhược điểm:

- Mỗi record có thể phải đọc lại root và internal nodes.
- Số lần disk read tăng rất nhanh khi range lớn.
- Chậm hơn B+ Tree Style khi dữ liệu lớn.

---

## 10.4 Benchmark

Cần chạy benchmark với các kích thước dữ liệu:

- 1,000
- 3,000
- 10,000
- 30,000
- 100,000
- 300,000
- 1,000,000
- 3,000,000
- 10,000,000

Với mỗi kích thước N, chạy 8 trường hợp:

Point Query:

- B-Tree Style + Linear Search
- B-Tree Style + Binary Search
- B+ Tree Style + Linear Search
- B+ Tree Style + Binary Search

Range Query:

- B-Tree Style + Linear Search
- B-Tree Style + Binary Search
- B+ Tree Style + Linear Search
- B+ Tree Style + Binary Search

Mỗi trường hợp chạy 100 query và lấy trung bình:

- Mean Time (ns)
- Mean Disk Reads

Trước mỗi query cần reset:

- disk_read_count = 0
- bộ đo thời gian bằng chrono

Chỉ đo thời gian query, không đo thời gian đọc CSV hoặc build index.

# 11. Kiểm thử, kết quả mong đợi và phân tích báo cáo

## 11.1 Kiểm thử đúng sai

Trước khi benchmark dữ liệu lớn, cần kiểm thử với dữ liệu nhỏ.

Các test cơ bản:

- Insert 1 record vào cây rỗng.
- Insert nhiều record nhưng chưa split leaf.
- Insert đủ nhiều để xảy ra leaf split.
- Insert đủ nhiều để xảy ra internal split.
- Insert đủ nhiều để xảy ra root split.
- Kiểm tra header.root_offset sau khi root split.
- Kiểm tra next_leaf_offset sau khi leaf split.
- Query id tồn tại.
- Query id không tồn tại.
- Range query trên một leaf.
- Range query đi qua nhiều leaf.

Kết quả cần đảm bảo:

- Các key trong mỗi node luôn tăng dần.
- Tất cả record thật chỉ nằm ở leaf.
- Internal node chỉ dùng để điều hướng.
- Leaf linked list không bị đứt.
- Không đọc hoặc ghi lệch page 4096 bytes.

---

## 11.2 Kết quả mong đợi

Point Query:

- Disk Reads xấp xỉ chiều cao của cây.
- Khi N tăng, số lần đọc đĩa tăng chậm vì B+ Tree có branching factor lớn.
- Binary Search thường nhanh hơn Linear Search về CPU time, nhưng Disk Reads gần như không đổi.

Range Query:

- B+ Tree Style phải nhanh hơn rõ rệt so với B-Tree Style.
- B+ Tree Style tận dụng next_leaf_offset để scan leaf liên tiếp.
- B-Tree Style bị chậm vì phải lặp lại traversal từ root cho nhiều key.

Với dữ liệu rất lớn:

- Disk I/O là bottleneck chính.
- Chênh lệch giữa B+ Tree Style và B-Tree Style thể hiện rõ nhất ở range query.
- Chênh lệch giữa Linear Search và Binary Search chủ yếu thể hiện ở thời gian CPU, không phải số lần đọc đĩa.

---

## 11.3 Nội dung cần đưa vào Report

Report cần có:

- Mô tả cấu trúc Record, BPlusNode, DBHeader.
- Giải thích disk layout theo page 4096 bytes.
- Trình bày thuật toán insert, leaf split, internal split, root split.
- Trình bày thuật toán point query và range query.
- Bảng kết quả benchmark cho point query.
- Bảng kết quả benchmark cho range query.
- Ít nhất 2 biểu đồ log-scale.
- Phân tích vì sao B+ Tree tối ưu cho range query.
- Phân tích ảnh hưởng của branching factor tới chiều cao cây.
- Phân tích CPU-bound và I/O-bound.
- Phân tích fragmentation khi insert random records.

---

## 11.4 Lưu ý khi nộp bài

Không nộp:

- dataset_large.csv
- index.dat
- index_N.dat
- file .exe
- file .o
- build directory

Chỉ nộp:

- source code trong thư mục Solution/
- Report.pdf

Tên file nộp theo format:

[StudentID1]_[StudentID2]_Lab1.zip
