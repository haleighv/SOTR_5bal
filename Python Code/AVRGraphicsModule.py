############################################
#
# AVRGRaphicsModule.py
# Written by Doug Gallatin and Andrew Lehmer 2012
#
# Code provided as is.  Use and Modify at your own risk.
# Packaged and tested using python2.7 32 bit, pygame 1.9.1, pySerial 2.6
#
############################################

import pygame
from pygame import event, display
import sys, os, imp
from threading import Thread, Semaphore
from serial import Serial

import AVRConstants as const
from AVRConstants import INT8, INT16, STRING
from AVRSprite import AVRSprite
from AVRGroup import AVRGroup

class AVRInterface(object):
	class exception(Exception):
		pass
	
	def __init__(self):
		if len(sys.argv) < 2:
			print "usage: AVRInterface <COM_PORT>"
			return
	
		pygame.init()
		self.displayInit = Semaphore(0)
		self.windowInit = Semaphore(0)
		self.running = True					#set to false if window is destroyed; stops sensor polling thread
		
		self.sensor = Serial(port=sys.argv[1], baudrate=const.BAUD_RATE, timeout=1)
		self.sensor.write(chr(0xff))
		print "Sent initialization 0xff"
		
		#function command to the python handle function and the argument types it takes
		self.mapping = {
			const.CREATE_SPRITE: [self.onCreateSprite, [STRING, INT16, INT16, INT16, INT16, INT16, INT8]],
			const.SET_POS: [self.onSetPos, [INT8, INT16, INT16]],
			const.SET_ROT: [self.onSetRot, [INT8, INT16]],
			const.SET_ORDER: [self.onSetOrder, [INT8, INT8]],
			const.SET_SIZE: [self.onSetSize, [INT8, INT16, INT16]],
			const.DELETE_SPRITE: [self.onDeleteSprite, [INT8]],
			const.CREATE_GROUP: [self.onCreateGroup, []],
			const.ADD_TO_GROUP: [self.onAddToGroup, [INT8, INT8]],
			const.REMOVE_FROM_GROUP: [self.onRemoveFromGroup, [INT8, INT8]],
			const.DELETE_GROUP: [self.onDeleteGroup, [INT8]],
			const.COLLIDE: [self.onCollide, [INT8, INT8]],
			const.CREATE_WINDOW: [self.onCreateWindow, [INT16, INT16]],
			const.PRINT: [self.onPrint, [STRING]],
		}
		
		self.run()
	
	def run(self):		
		Thread(target=self.pollAVR).start()
		
		#wait for the AVR to call CREATE_WINDOW
		self.windowInit.acquire()
		
		self.disp = display.set_mode((self.width,self.height))
		self.back = pygame.Surface((self.width,self.height))
		self.back.fill((0,0,0), pygame.Rect(0,0,self.width, self.height))
		
		self.displayInit.release()
		
		self.pygameMainloop()
	
	def pygameMainloop(self):
		print "Starting Render Loop"
		
		while self.running:
			for e in event.get():
				if e.type == pygame.QUIT: 
					self.running = False
					sys.exit()
					
			#clear then delete things atomically
			AVRSprite.deleteLock.acquire()
			AVRSprite.spriteDrawGroup.clear(self.disp, self.back)
			AVRSprite.onDelete()
			AVRSprite.deleteLock.release()
			
			AVRSprite.updateGraphics()
			
			AVRSprite.spriteLock.acquire()
			display.update(AVRSprite.spriteDrawGroup.draw(self.disp))
			AVRSprite.spriteLock.release()
			
			AVRSprite.update()
	
	def onCreateSprite(self, file, x, y, angle, w, h, order):
		try:
			s = AVRSprite(file, (x,y), angle, (w,h), order)
		except pygame.error:
			return const.HANDLE_ERROR
		if s.handle == -1:
			return const.HANDLE_ERROR
		return s.handle		
	
	def onSetPos(self, handle, x, y):
		if handle in AVRSprite.spriteList:
			AVRSprite.spriteList[handle].setPos((x,y))
		else:
			print "setPos: Unknown handle %d" % handle
			raise AVRInterface.exception('onSetPos')
		return -1
	
	def onSetRot(self, handle, angle):
		if handle in AVRSprite.spriteList:
			AVRSprite.spriteList[handle].setAngle(angle)
		else:
			print "setAngle: Unknown handle %d" % handle
			raise AVRInterface.exception('onSetRot')
		return -1
	
	def onSetSize(self, handle, x, y):
		if handle in AVRSprite.spriteList:
			AVRSprite.spriteList[handle].setSize((x,y))
		else:
			print "setSize: Unknown handle %d" % handle
			raise AVRInterface.exception('onSetSize')
		return -1
	
	def onSetOrder(self, handle, order):
		if handle in AVRSprite.spriteList:
			AVRSprite.spriteList[handle].setOrder(order)
		else:
			print "setOrder: Unknown handle %d" % handle
			raise AVRInterface.exception('onSetOrder')
		return -1
	
	def onDeleteSprite(self, handle):
		if handle in AVRSprite.spriteList:
			AVRSprite.spriteList[handle].delete()
		else:
			print "deleteSprite: Unknown handle %d" % handle
			raise AVRInterface.exception('deleteSprite')
		return -1
		
	def onCreateGroup(self):
		g = AVRGroup()
		if g.handle == -1:
			return const.HANDLE_ERROR
		return g.handle
		
	def onAddToGroup(self, groupHandle, spriteHandle):
		if groupHandle not in AVRGroup.groupList:
			print "addToGroup: Unknown group handle %d" % groupHandle
			raise AVRInterface.exception('onAddToGroup')
		elif spriteHandle not in AVRSprite.spriteList:
			print "addToGroup: Unknown sprite handle %d" % spriteHandle
			raise AVRInterface.exception('onAddToGroup')
		else:
			AVRGroup.groupList[groupHandle].addSprite(AVRSprite.spriteList[spriteHandle])
		return -1
		
	def onRemoveFromGroup(self, groupHandle, spriteHandle):
		if groupHandle not in AVRGroup.groupList:
			print "removeFromGroup: Unknown group handle %d" % groupHandle
			raise AVRInterface.exception('onRemoveFromGroup')
		elif spriteHandle not in AVRSprite.spriteList:
			print "removeFromGroup: Unknown sprite handle %d" % spriteHandle
			raise AVRInterface.exception('onRemoveFromGroup')
		else:
			AVRGroup.groupList[groupHandle].removeSprite(AVRSprite.spriteList[spriteHandle])
		return -1
		
	def onDeleteGroup(self, handle):
		if handle in AVRGroup.groupList:
			AVRGroup.groupList[handle].delete()
		else:
			raise AVRInterface.exception('onDeleteGroup')
		return -1
		
	def onCollide(self, spriteHandle, groupHandle):
		if groupHandle not in AVRGroup.groupList:
			print "collide: Unknown group handle %d" % groupHandle
			raise AVRInterface.exception('onCollide')
		elif spriteHandle not in AVRSprite.spriteList:
			print "collide: Unknown sprite handle %d" % spriteHandle
			raise AVRInterface.exception('onCollide')
		else:
			return AVRSprite.spriteList[spriteHandle].collide(AVRGroup.groupList[groupHandle])
		return []
		
	def onCreateWindow(self, w, h):
		self.width, self.height = w, h
		self.windowInit.release()
		self.displayInit.acquire()
		
		return -1
	
	def onPrint(self, s):
		print s
		return -1
	
	def pollAVR(self):
        #read garbage bit from board to sync
		self.sensor.read(1)

		while (self.running):
			command = self.sensor.read(1)
			if len(command) != 1:
				continue
			command = ord(command)
			#print command
			#print command
			if command not in self.mapping:
				print "Command %s not recognized!" % command
				self.running = False
				return
				
			args = []
			for arg in self.mapping[command][1]:
				if arg == const.STRING:
					data = ''
					while True:
						d = self.sensor.read(1)
						print "got: " + str(d)
						if len(d) != 1:
							continue
						if ord(d[0]) == 0x00:
							break
						data += d[0]
					args.append(data)
				else:
					#read in 1 byte at a time and convert to appropriate sized integer
					data = []
					while len(data) != arg:
						d = self.sensor.read(1)
						print "got: " + str(d)
						if len(d) == 1:
							data.extend(d)
					arg = 0
					#receive high byte first, process low byte first
					data.reverse()
					for i, b in enumerate(data):
						arg |= (ord(b) << (i*8)) & (0xff << i*8)
					args.append(arg)	
			try:
				result = self.mapping[command][0](*args)
			except AVRInterface.exception as e:
				print "Exception:", e
				self.running = False
				sys.exit()
				
			if isinstance(result, list):
				for r in result:
					self.sensor.write(chr(r & 0xff))
				self.sensor.write(chr(0xff))
			else:
				if result != -1:
					self.sensor.write(chr(result & 0xff))
					
if __name__ == '__main__':
	AVRInterface()
