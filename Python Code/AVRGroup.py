############################################
#
# AVRGroup.py
# Written by Doug Gallatin and Andrew Lehmer 2012
#
# Code provided as is.  Use and Modify at your own risk.
# Packaged and tested using python2.7 32 bit, pygame 1.9.1, pySerial 2.6
#
############################################

from pygame import sprite
import AVRConstants as const
import AVRSprite

class AVRGroup(object):
	availableHandles = [i for i in range(0xFE)]
	groupList = {}
	
	def __init__(self, handle=None):
		if len(AVRGroup.availableHandles) == 0:
			print "ERROR: No available group handles"
			self.handle = -1
			return

		if handle == None:
			self.handle = AVRSprite.AVRSprite.availableHandles.pop()
		else:
			self.handle = const.ALL_GROUP
		AVRGroup.groupList[self.handle] = self
		
		self.group = sprite.Group()
		self.sprites = []
	
	def addSprite(self, sprite):
		if sprite in self.sprites:
			return
			
		self.group.add(sprite.sprite)
		self.sprites.append(sprite)
		sprite.groups.append(self)
	
	def removeSprite(self, sprite):
		if sprite not in self.sprites:
			return
		
		self.group.remove(sprite.sprite)
		self.sprites.remove(sprite)
		sprite.groups.remove(self)
	
	def delete(self):
		del AVRGroup.groupList[self.handle]
		AVRGroup.availableHandles.append(self.handle)
		
		for sprite in self.sprites:
			self.removeSprite(sprite)
		self.sprites = []
		
#instantiate ALL group
AVRGroup(const.ALL_GROUP)