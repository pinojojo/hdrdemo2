from PIL import Image, ImageDraw

# 定义图像的宽度和高度
width, height = 1920, 1080

# 创建一个新的RGB图像，底色为黑色
img = Image.new("RGB", (width, height), "black")
draw = ImageDraw.Draw(img)

# 棋盘格每个格子的大小和棋盘的行列数
cell_size = 8
rows, cols = 4, 5

# 计算棋盘的总宽度和高度
board_width = cell_size * cols
board_height = cell_size * rows

# 计算棋盘起始位置，以居中显示
start_x = (width - board_width) // 2
start_y = (height - board_height) // 2

# 绘制棋盘格
for row in range(rows):
    for col in range(cols):
        # 计算当前格子的左上角坐标
        x0 = start_x + col * cell_size
        y0 = start_y + row * cell_size
        x1 = x0 + cell_size
        y1 = y0 + cell_size
        # 根据行列号的奇偶性决定填充颜色
        if (row + col) % 2 == 0:
            draw.rectangle([x0, y0, x1, y1], fill="white")

# 保存图像为BMP格式
img.save("chessboard.bmp")
