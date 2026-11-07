"""The training pad and each respawn mesh as text, and back as the bytes the
game copies. A float is written as its bits, so the text is the same mesh."""

import struct


_RECORDS = {"bitmap", "vertex", "indices", "pixels", "place", "node", "part"}


def _tokens(text):
    for line in text.splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            yield from line.split()


def _bits(word):
    return f"0x{word & 0xFFFFFFFF:08x}"


def _take_bits(token):
    if len(token) != 10 or not token.startswith("0x"):
        raise ValueError(f"expected a float's bits, got {token}")
    return int(token, 16)


def _vertex_line(vertex):
    words = struct.unpack("<14I2h2I", vertex)
    names = ("position", "normal", "binormal", "tangent")
    parts = []
    for index, name in enumerate(names):
        start = index * 3
        parts.append(name + " " + " ".join(_bits(word) for word in words[start:start + 3]))
    parts.append("texcoord " + " ".join(_bits(word) for word in words[12:14]))
    parts.append(f"nodes {words[14]} {words[15]}")
    parts.append("node_weights " + " ".join(_bits(word) for word in words[16:18]))
    return "vertex " + " ".join(parts)


def _named_bits(prefix, names, data, offset):
    words = struct.unpack_from(f"<{len(names)}I", data, offset)
    fields = " ".join(f"{name} {_bits(word)}" for name, word in zip(names, words))
    return f"{prefix} {fields}", offset + len(names) * 4


def _u16(values):
    out = bytearray()
    for start in range(0, len(values), 4096):
        chunk = values[start:start + 4096]
        if chunk:
            out += struct.pack(f"<{len(chunk)}H", *chunk)
    return bytes(out)


def marker_text(payload):
    """The pad payload as the bitmap struct, each vertex struct, the strip, and the pixels."""
    vertex_count, index_count, pixel_bytes = struct.unpack_from("<3I", payload)
    offset = 12
    bitmap = payload[offset:offset + 0x30]
    offset += 0x30
    vertices = payload[offset:offset + vertex_count * 0x44]
    offset += vertex_count * 0x44
    indices = struct.unpack_from(f"<{index_count}H", payload, offset)
    offset += index_count * 2
    pixels = payload[offset:offset + pixel_bytes]
    if offset + pixel_bytes != len(payload) or len(bitmap) != 0x30 or len(vertices) != vertex_count * 0x44:
        raise ValueError("marker payload does not match its counts")
    signature, width, height, depth, kind, image_format, flags = struct.unpack_from("<I5hH", bitmap)
    registration_x, registration_y, mipmaps, mipmap_pad = struct.unpack_from("<4h", bitmap, 16)
    pixels_offset, pixels_size, tag_index, cache_block = struct.unpack_from("<4i", bitmap, 24)
    hardware, base = struct.unpack_from("<2I", bitmap, 40)
    lines = [
        "# The pad. Floats are their exact bits. Fields follow bitmap_data and model_vertex_uncompressed.",
        "marker",
        "bitmap "
        f"signature {_bits(signature)} width {width} height {height} depth {depth} "
        f"type {kind} format {image_format} flags {flags} "
        f"registration {registration_x} {registration_y} mipmaps {mipmaps} mipmap_pad {mipmap_pad} "
        f"pixels_offset {pixels_offset} pixels_size {pixels_size} tag_index {tag_index} "
        f"cache_block_index {cache_block} hardware_format {_bits(hardware)} base_address {_bits(base)}",
    ]
    for index in range(vertex_count):
        lines.append(_vertex_line(vertices[index * 0x44:(index + 1) * 0x44]))
    for start in range(0, index_count, 16):
        lines.append("indices " + " ".join(str(index) for index in indices[start:start + 16]))
    for start in range(0, len(pixels), 16):
        lines.append("pixels " + " ".join(f"{byte:02x}" for byte in pixels[start:start + 16]))
    return "\n".join(lines) + "\n"


def area_text(payload):
    """One map's respawn mesh: the places, the node, and each part's vertices and strip."""
    place_count, = struct.unpack_from("<I", payload)
    offset = 4
    lines = [
        "# One respawn mesh. Floats are their exact bits. A vertex follows model_vertex_uncompressed.",
        "area",
    ]
    for _ in range(place_count):
        line, offset = _named_bits("place", ("x", "y", "z", "yaw", "pitch", "roll"), payload, offset)
        lines.append(line)
    line, offset = _named_bits("node", ("x", "y", "z"), payload, offset)
    lines.append(line)
    part_count, = struct.unpack_from("<I", payload, offset)
    offset += 4
    for _ in range(part_count):
        vertex_count, index_count = struct.unpack_from("<2I", payload, offset)
        offset += 8
        lines.append("part")
        for index in range(vertex_count):
            lines.append(_vertex_line(payload[offset:offset + 0x44]))
            offset += 0x44
        indices = struct.unpack_from(f"<{index_count}H", payload, offset)
        offset += index_count * 2
        for start in range(0, index_count, 16):
            lines.append("indices " + " ".join(str(index) for index in indices[start:start + 16]))
    if offset != len(payload):
        raise ValueError("area payload does not match its counts")
    return "\n".join(lines) + "\n"


def _parse(text):
    tokens = list(_tokens(text))
    index = 0

    def take(name=None):
        nonlocal index
        if index >= len(tokens):
            raise ValueError(f"expected {name or 'more'}, the file ended")
        token = tokens[index]
        index += 1
        if name is not None and token != name:
            raise ValueError(f"expected {name}, got {token}")
        return token

    def number(name):
        take(name)
        return int(take())

    def bits(name):
        take(name)
        return _take_bits(take())

    def gather(convert):
        values = []
        while index < len(tokens) and tokens[index] not in _RECORDS:
            values.append(convert(take()))
        return values

    return tokens, take, number, bits, gather, lambda: index < len(tokens)


def text_to_payload(text):
    """The bytes the game copies, from a marker or area text file."""
    tokens, take, number, bits, gather, more = _parse(text)
    kind = take()
    if kind == "marker":
        return _marker(take, number, bits, gather, more)
    if kind == "area":
        return _area(take, number, bits, gather, more)
    raise ValueError(f"expected marker or area, got {kind}")


def _marker(take, number, bits, gather, more):
    take("bitmap")
    signature = bits("signature")
    width = number("width")
    height = number("height")
    depth = number("depth")
    kind = number("type")
    image_format = number("format")
    flags = number("flags")
    take("registration")
    registration = (int(take()), int(take()))
    mipmaps = number("mipmaps")
    mipmap_pad = number("mipmap_pad")
    pixels_offset = number("pixels_offset")
    pixels_size = number("pixels_size")
    tag_index = number("tag_index")
    cache_block = number("cache_block_index")
    hardware = bits("hardware_format")
    base = bits("base_address")
    bitmap = struct.pack(
        "<I5hH4h4i2I", signature, width, height, depth, kind, image_format, flags,
        registration[0], registration[1], mipmaps, mipmap_pad,
        pixels_offset, pixels_size, tag_index, cache_block, hardware, base)
    vertices = bytearray()
    indices = []
    pixels = bytearray()
    while more():
        record = take()
        if record == "vertex":
            vertices += _vertex(take, number, bits)
        elif record == "indices":
            indices.extend(gather(int))
        elif record == "pixels":
            pixels += bytes(gather(lambda token: int(token, 16)))
        else:
            raise ValueError(f"unexpected {record} in a marker")
    return struct.pack("<3I", len(vertices) // 0x44, len(indices), len(pixels)) + bitmap + vertices + _u16(indices) + bytes(pixels)


def _area(take, number, bits, gather, more):
    places = bytearray()
    node = None
    parts = []
    vertices = None
    indices = None
    while more():
        record = take()
        if record == "place":
            places += struct.pack("<6I", *(bits(name) for name in ("x", "y", "z", "yaw", "pitch", "roll")))
        elif record == "node":
            node = struct.pack("<3I", *(bits(name) for name in ("x", "y", "z")))
        elif record == "part":
            if vertices is not None:
                parts.append((vertices, indices))
            vertices = bytearray()
            indices = []
        elif record == "vertex":
            vertices += _vertex(take, number, bits)
        elif record == "indices":
            indices.extend(gather(int))
        else:
            raise ValueError(f"unexpected {record} in an area")
    if vertices is not None:
        parts.append((vertices, indices))
    if node is None or not parts:
        raise ValueError("an area needs a node and a part")
    payload = struct.pack("<I", len(places) // 24) + bytes(places) + node + struct.pack("<I", len(parts))
    for part_vertices, part_indices in parts:
        payload += struct.pack("<2I", len(part_vertices) // 0x44, len(part_indices))
        payload += part_vertices
        payload += _u16(part_indices)
    return payload


def _vertex(take, number, bits):
    words = []
    for name in ("position", "normal", "binormal", "tangent"):
        take(name)
        words.extend(_take_bits(take()) for _ in range(3))
    take("texcoord")
    words.extend(_take_bits(take()) for _ in range(2))
    take("nodes")
    nodes = (int(take()), int(take()))
    take("node_weights")
    weights = (_take_bits(take()), _take_bits(take()))
    return struct.pack("<14I2h2I", *words, nodes[0], nodes[1], *weights)
