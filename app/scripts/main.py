import wx
import random
import os
from linux_usb_serial import * #usb_interface, map_loop
import pdb

global pingFrequency
global ZOOM_LEVEL

ZOOM_LEVEL = 18
# default value

def scale_bitmap(bitmap, width, height):
    image = wx.ImageFromBitmap(bitmap)
    image = image.Scale(width, height, wx.IMAGE_QUALITY_HIGH)
    result = wx.BitmapFromImage(image)
    return result

############# First Panel, Main App Screen ###############################
class PanelOne(wx.Panel):
    """"""

    #----------------------------------------------------------------------
    def __init__(self, parent):
        """Constructor"""
        wx.Panel.__init__(self, parent=parent)
        self.SetBackgroundColour('gray')

        hbox = wx.BoxSizer(wx.HORIZONTAL)

        fgs = wx.FlexGridSizer(3, 5, 9, 25)

        title = wx.StaticText(self, label="Fiercely Efficient Tracking CHip")
        separator = wx.StaticLine(self)
        separator2 = wx.StaticLine(self)
        self.findButton = wx.Button(self, label="Find Me!")
        self.Bind(wx.EVT_BUTTON, self.onFindButtonClick, self.findButton)
        self.sleepButton = wx.Button(self, label="Sleep")
        self.Bind(wx.EVT_BUTTON, self.onSleepButtonClick, self.sleepButton)
        fetchImage = wx.Bitmap('FETCH-logo.png')
        fetchImage = scale_bitmap(fetchImage, 100, 100)
        icon = wx.StaticBitmap(self, bitmap=fetchImage)

        localizeHeader = wx.StaticText(self, label="Localize time: ")
        self.localizeTime = wx.TextCtrl(self)
        sleepHeader = wx.StaticText(self, label="Sleep time: ")
        self.sleepTime = wx.TextCtrl(self)

        zoomText = wx.StaticText(self, label="Zoom: ")
        self.zoomCtrl = wx.SpinCtrl(self, value='18')
        self.zoomCtrl.SetRange(1, 20)
        self.Bind(wx.EVT_SPINCTRL, self.onZoomCtrlClick, self.zoomCtrl)

        fgs.AddMany([(icon), (title, 1, wx.EXPAND), (separator),
            (self.findButton, 1, wx.EXPAND),(self.sleepButton, 1, wx.EXPAND),
            (localizeHeader), (self.localizeTime), (sleepHeader),
            (self.sleepTime), (separator2), (zoomText), (self.zoomCtrl)])

        # fgs.AddGrowableRow(2, 1)
        # sfgs.AddGrowableCol(1, 1)

        hbox.Add(fgs, proportion=1, flag=wx.ALL|wx.EXPAND, border=15)
        self.SetSizer(hbox)

    def onFindButtonClick(self, e):
        sleep_time = self.localizeTime.GetValue()
        set_current_command_from_gui('l' + str(sleep_time))

    def onSleepButtonClick(self, e):
        sleep_time = self.sleepTime.GetValue()
        set_current_command_from_gui('s' + str(sleep_time))

    def onZoomCtrlClick(self, e):
        zoom = self.zoomCtrl.GetValue()
        set_current_zoom(zoom)
        request_map_update()

############### Second Panel: Map Func.####################################
class PanelTwo(wx.Panel):
    """"""

    #----------------------------------------------------------------------
    def __init__(self, parent):
        """Constructor"""
        wx.Panel.__init__(self, parent=parent)

        self.SetBackgroundColour('yellow')

        hbox = wx.BoxSizer(wx.HORIZONTAL)

        fgs = wx.FlexGridSizer(4, 2, 9, 25)

        title = wx.StaticText(self, label="Fiercely Efficient Tracking CHip")
        separator = wx.StaticLine(self)
        self.findButton = wx.Button(self, label="Find Me!")
        zoomText = wx.StaticText(self, label="Zoom: ")

        fetchImage = wx.Bitmap('FETCH-logo.png')
        fetchImage = scale_bitmap(fetchImage, 100, 100)
        icon = wx.StaticBitmap(self, bitmap=fetchImage)

        separator2 = wx.StaticLine(self)
        self.zoomCtrl = wx.SpinCtrl(self, value='10')
        self.zoomCtrl.SetRange(1, 20)


        fgs.AddMany([(icon), (title, 1, wx.EXPAND), (separator),
            (self.findButton, 1, wx.EXPAND), (zoomText), (separator2),
            (self.zoomCtrl)])

        fgs.AddGrowableRow(2, 1)
        fgs.AddGrowableCol(1, 1)

        hbox.Add(fgs, proportion=1, flag=wx.ALL|wx.EXPAND, border=15)
        self.SetSizer(hbox)


################# Main Frame ########################################
class Example(wx.Frame):

    #----------------------------------------------------------------------
    def __init__(self):
        wx.Frame.__init__(self, None, wx.ID_ANY,
                          "Panel Switcher Tutorial")

        self.panel_one = PanelOne(self)
        self.panel_two = PanelTwo(self)
        self.panel_two.Hide()

        self.sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer.Add(self.panel_one, 1, wx.EXPAND)
        self.sizer.Add(self.panel_two, 1, wx.EXPAND)
        self.SetSizer(self.sizer)


        menubar = wx.MenuBar()
        fileMenu = wx.Menu()
        switch_panels_menu_item = fileMenu.Append(wx.ID_ANY,
                                                  "Switch Panel",
                                                  "Some text")
        self.Bind(wx.EVT_MENU, self.onSwitchPanels,
                  switch_panels_menu_item)
        menubar.Append(fileMenu, '&File')
        self.SetMenuBar(menubar)

    #----------------------------------------------------------------------


    def onSwitchPanels(self, event):
        """"""
        if self.panel_one.IsShown():
            self.SetTitle("Fetching Device")
            self.panel_one.Hide()
            self.panel_two.Show()
        else:
            self.SetTitle("FETCH")
            self.panel_one.Show()
            self.panel_two.Hide()
        self.Layout()

def main():
    serial_thread = threading.Thread(target=usb_interface)
    serial_thread.daemon = False
    serial_thread.start()
    
    plotting_thread = threading.Thread(target=map_loop)
    plotting_thread.daemon = False
    plotting_thread.start()

    gui_thread = threading.Thread(target=gui)
    gui_thread.daemon = False
    gui_thread.start()
    #gui()

    def stop(a, b):
        global STOP
        STOP = True
        raise Exception

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

def gui():
    app = wx.App()
    ex = Example()
    ex.Show()
    app.MainLoop()

if __name__ == '__main__':
    main()