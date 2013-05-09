from pygame import image, error, transform, sprite, mask
import AVRGroup
import AVRConstants as const
from threading import Lock
'''possible to load:
JPG 
 PNG 
 GIF (non animated) 
 BMP 
 PCX 
 TGA (uncompressed) 
 TIF 
 LBM (and PBM) 
 PBM (and PGM, PPM) 
 XPM
 '''

class AVRSprite(object):
	availableHandles = [i for i in range(0xFE)]
	spriteList = {}
	deletedSprites = []
	spriteDrawGroup = sprite.LayeredDirty()
	deleteLock = Lock()
	spriteLock = Lock()
	
	def __init__(self, filename, pos, angle, size, order):
		self.pos = pos
		self.angle = angle
		self.size = size[:]	#copy	
		self.order = order
		self.filename = filename
		self.groups = []
		
		if len(AVRSprite.availableHandles) == 0:
			print "ERROR: AVRSprite out of handles!"
			self.handle = -1
			return
		
		try:
			self.surface = image.load(self.filename).convert_alpha()
		except error as e:
			print "ERROR: Could not load image: '%s'" % self.filename
			raise e
		
		self.scaledSurface = transform.smoothscale(self.surface, self.size)
		self.transformedSurface = transform.rotate(self.scaledSurface, self.angle)  
		
		self.sprite = sprite.DirtySprite()
		self.sprite.image = self.transformedSurface
		self.sprite.rect = self.transformedSurface.get_rect()
		self.sprite.rect.center = self.pos
		self.sprite.dirty = 1
		self.sprite.maskDirty = True
		self.sprite.AVRSprite = self
		
		self.posDirty = False
		self.rotateDirty = False
		self.sizeDirty = False
		
		AVRSprite.spriteDrawGroup.add(self.sprite, layer=self.order)
		AVRGroup.AVRGroup.groupList[const.ALL_GROUP].addSprite(self)
		self.handle = AVRSprite.availableHandles.pop()
		AVRSprite.spriteList[self.handle] = self
		
		#print 'sprite %s with handle %s' % (self.filename, self.handle)
		
	def setOrder(self, order):			
		self.order = order
		AVRSprite.spriteDrawGroup.change_layer(self.sprite, order)
		self.sprite.dirty = 1
	
	def setPos(self, pos):
		self.pos = pos
		self.posDirty = True
	
	def setAngle(self, angle):
		if angle == self.angle:
			return
			
		self.angle = angle
		self.rotateDirty = True
	
	def setSize(self, size):
		if size[0] == self.size[0] and size[1] == self.size[1]:
			return
		self.size = size[:]
		
		self.sizeDirty = True
				
	def delete(self):
		#locks required because in order to delete, 
		#the main thread must clear and then call AVRSprite.onDelete atomically
		#also, this gaurantees the order of object deletion so no infinite reference loops occur
		AVRSprite.deleteLock.acquire()
		AVRSprite.deletedSprites.append(self)
		self.sprite.dirty = 1
		for g in self.groups[:]:
			g.removeSprite(self)
		self.groups = []
		
		self.sprite.AVRSprite = None
		del AVRSprite.spriteList[self.handle]
		AVRSprite.availableHandles.append(self.handle)
		AVRSprite.deleteLock.release()
	
	def collide(self, group):
		if self.sprite.maskDirty:
			self.sprite.maskDirty = False
			AVRSprite.spriteLock.acquire()
			self.sprite.mask = mask.from_surface(self.transformedSurface)
			AVRSprite.spriteLock.release()
		
		sprites = sprite.spritecollide(self.sprite, group.group, False)
		results = []
		for s in sprites:
			if s == self.sprite:
				continue
			if s.maskDirty:
				s.maskDirty = False
				AVRSprite.spriteLock.acquire()
				s.mask = mask.from_surface(s.AVRSprite.transformedSurface)
				AVRSprite.spriteLock.release()
			if sprite.collide_mask(self.sprite, s) != None:
				results.append(s.AVRSprite.handle)
		return results
	
	@staticmethod
	def onDelete():
		for s in AVRSprite.deletedSprites:
			AVRSprite.spriteDrawGroup.remove(s.sprite)
			s.sprite = None
			
		AVRSprite.deletedSprites = []
		
	@staticmethod
	def update():
		for s in AVRSprite.spriteList.values():
			if s.posDirty or s.sizeDirty or s.rotateDirty:
				s.sprite.dirty = 1
	
	@staticmethod
	def updateGraphics():
		for s in AVRSprite.spriteList.values():
			if s.sizeDirty:
				s.scaledSurface = transform.smoothscale(s.surface, s.size)
				s.transformedSurface = transform.rotate(s.scaledSurface, s.angle)
				s.sprite.image = s.transformedSurface
				s.sprite.rect = s.transformedSurface.get_rect()
				s.sprite.rect.center = s.pos
				s.sprite.maskDirty = True
				
			elif s.rotateDirty:
				s.transformedSurface = transform.rotate(s.scaledSurface, s.angle)
				s.sprite.image = s.transformedSurface
				s.sprite.rect = s.transformedSurface.get_rect()
				s.sprite.rect.center = s.pos
				s.sprite.maskDirty = True
				
			elif s.posDirty:
				s.sprite.rect.center = s.pos
				s.posDirty = False
			s.posDirty = True
			s.rotateDirty = True
			s.sizeDirty = True
