import wx
import random
import scripts
import os

global pingFrequency

def scale_bitmap(bitmap, width, height):
    image = wx.ImageFromBitmap(bitmap)
    image = image.Scale(width, height, wx.IMAGE_QUALITY_HIGH)
    result = wx.BitmapFromImage(image)
    return result


class Example(wx.Frame):

    def __init__(self, parent, title):
        super(Example, self).__init__(parent, title=title)

        self.InitUI()
        self.Centre()

    def InitUI(self):

        panel = wx.Panel(self)
        panel.SetBackgroundColour("gray")
        menubar = wx.MenuBar()

        # eventual functionality
        fileMenu = wx.Menu()
        fileMenu.Append(wx.ID_NEW, '&Add Gateway')
        fileMenu.Append(wx.ID_OPEN, '&Add Device')
        fileMenu.Append(wx.ID_SAVE, '&About Fetch')
        fileMenu.AppendSeparator()

        imp = wx.Menu()
        imp.Append(wx.ID_ANY, 'Import location data...')

        fileMenu.AppendMenu(wx.ID_ANY, 'I&mport', imp)

        qmi = wx.MenuItem(fileMenu, wx.ID_EXIT, '&Quit\tCtrl+W')
        fileMenu.AppendItem(qmi)

        self.Bind(wx.EVT_MENU, self.OnQuit, qmi)

        menubar.Append(fileMenu, '&File')
        self.SetMenuBar(menubar)

        sizer = wx.GridBagSizer(7,7)

        appHeader = wx.StaticText(panel, label="Fetch: Fiercly Efficient Tracking CHip")
        sizer.Add(appHeader, pos=(0,0), flag=wx.TOP|wx.LEFT|wx.BOTTOM,
            border=15)

        #      bitmap = wx.Bitmap(path)
        # bitmap = scale_bitmap(bitmap, 300, 200)
        # control = wx.StaticBitmap(self, -1, bitmap)
        fetchImage = wx.Bitmap('fetch-image.png')
        fetchImage = scale_bitmap(fetchImage, 75, 75)

        icon = wx.StaticBitmap(panel, bitmap=fetchImage)
        sizer.Add(icon, pos=(0, 1), flag=wx.TOP|wx.RIGHT|wx.ALIGN_RIGHT,
            border=5)

        separator = wx.StaticLine(panel)
        sizer.Add(separator, pos=(1, 0), span=(1, 4),
            flag=wx.EXPAND|wx.BOTTOM, border=10)

        findButton = wx.Button(panel, label="Find Me!")
        sizer.Add(findButton, pos=(2,1), flag=wx.RIGHT, border=10)
        self.Bind(wx.EVT_BUTTON, self.onFindButtonClick)

        setPingFreqText = wx.StaticText(panel, label="Set ping frequency: ")
        sizer.Add(setPingFreqText, pos=(3, 0), flag=wx.TOP|wx.LEFT|wx.BOTTOM, border=5)

        pingFrequencyText = wx.TextCtrl(panel)
        sizer.Add(pingFrequencyText, pos=(3,1), span=(1,2), flag=wx.RIGHT|wx.BOTTOM, border=5)


        panel.SetSizer(sizer)
        sizer.Fit(self)

    def OnQuit(self, e):
        self.Close()

    def onFindButtonClick(self, e):
        print('starting function')
        os.system('python3 scripts/linux_usb_serial.py')


def main():
    app = wx.App()
    ex = Example(None, title="FETCH")
    ex.Show()
    app.MainLoop()


if __name__ == '__main__':
    main()
