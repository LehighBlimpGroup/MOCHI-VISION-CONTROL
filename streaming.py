import io, pygame, rpc, serial, serial.tools.list_ports, socket, sys

try: input = raw_input
except NameError: pass

interface = rpc.rpc_network_master(slave_ip='192.168.0.7', my_ip='', port=7610)

##############################################################
# Call Back Handlers
##############################################################

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

while(True):
    sys.stdout.flush()
    result = interface.call('jpeg_image_stream', 'sensor.RGB565,sensor.QQVGA')
    if result is not None:# THE REMOTE DEVICE WILL START STREAMING ON SUCCESS. SO, WE NEED TO RECEIVE DATA IMMEDIATELY.
        interface.stream_reader(jpg_frame_buffer_cb, queue_depth=8)
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            quit()

