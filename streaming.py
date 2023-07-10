import io, pygame, rpc, sys

def jpg_frame_buffer_cb(data):
    sys.stdout.flush()
    try:
        screen.blit(pygame.transform.scale(pygame.image.load(io.BytesIO(data), 'jpg'), (screen_w, screen_h)), (0, 0))
        pygame.display.update()
        clock.tick()
    except pygame.error: pass
    print(clock.get_fps())
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            quit()

if __name__ == "__main__":
    # interface that will be used to communicate with the remote device
    interface = rpc.rpc_network_master(slave_ip='192.168.0.115', my_ip='', port=0x1DBA)

    # initialize pygame
    pygame.init()
    screen_w = 640
    screen_h = 480
    # screen_w = 160
    # screen_h = 120
    try:
        screen = pygame.display.set_mode((screen_w, screen_h), flags=pygame.RESIZABLE)
    except TypeError:
        screen = pygame.display.set_mode((screen_w, screen_h))
    pygame.display.set_caption('Frame Buffer')
    clock = pygame.time.Clock()

    # start streaming
    while(True):
        sys.stdout.flush()
        result = interface.call('jpeg_image_stream', 'sensor.RGB565,sensor.QQVGA')
        print(result)
        if result is not None:# THE REMOTE DEVICE WILL START STREAMING ON SUCCESS. SO, WE NEED TO RECEIVE DATA IMMEDIATELY.
            print("Success")
            interface.stream_reader(jpg_frame_buffer_cb, queue_depth=8)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                quit()

