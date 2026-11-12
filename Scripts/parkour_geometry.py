"""Shared visible railings with simple, reliable vault collision."""
def fence(mesh, cube, metal, edge, label, x, y, ground=0, width=500, height=110):
    proxy=mesh(label+'_Collision',cube,(x,y,ground+height/2),(30,width,height),material=metal)
    proxy.set_actor_hidden_in_game(True)
    for side in [-1,1]:
        mesh(label+'_Post_'+str(side),cube,(x,y+side*width/2,ground+height/2),(14,14,height),material=metal,collision=False)
    for level in [25,65,height-5]:
        mesh(label+'_Rail_'+str(level),cube,(x,y,ground+level),(10,width,8),material=edge if level==height-5 else metal,collision=False)
    for i in range(1,int(width/50)):
        mesh(label+'_Slat_'+str(i),cube,(x,y-width/2+i*50,ground+height/2),(6,6,height-12),material=metal,collision=False)
    return proxy

def drain_escape(mesh,cube,metal,edge):
    """A visible stair from the low grated service pit back to street level."""
    for i in range(16):
        h=(i+1)*22.5
        mesh('Drain_Escape_Step_'+str(i),cube,(9600,-3145+i*26,-180+h/2),(110,26,h),material=metal)
    mesh('Drain_Escape_Landing',cube,(9600,-2700,170),(160,180,20),material=metal)
    mesh('Drain_Escape_Bridge',cube,(9300,-2850,112.35),(684.3,180,20),yaw=-153.435,pitch=-11.405,material=metal)
    mesh('Drain_Escape_StreetJoin',cube,(9000,-3000,43),(260,260,20),material=metal)
    for side in [-1,1]:
        mesh('Drain_Escape_Bridge_Edge_'+str(side),cube,(9300-side*35.8,-2850+side*71.6,125),(684.3,8,8),yaw=-153.435,pitch=-11.405,material=edge,collision=False)
    for side in [-1,1]:
        mesh('Drain_Escape_Edge_'+str(side),cube,(9600+side*51,-2950,91.25),(516,8,8),yaw=90,pitch=40.87,material=edge,collision=False)
