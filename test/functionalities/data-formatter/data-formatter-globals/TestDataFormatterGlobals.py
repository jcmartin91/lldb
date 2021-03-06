"""
Test lldb data formatter subsystem.
"""

import os, time
import unittest2
import lldb
from lldbtest import *

class GlobalsDataFormatterTestCase(TestBase):

    mydir = os.path.join("functionalities", "data-formatter", "data-formatter-globals")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dsym_test
    def test_with_dsym_and_run_command(self):
        """Test data formatter commands."""
        self.buildDsym()
        self.data_formatter_commands()

    @dwarf_test
    def test_with_dwarf_and_run_command(self):
        """Test data formatter commands."""
        self.buildDwarf()
        self.data_formatter_commands()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break at.
        self.line = line_number('main.cpp', '// Set break point at this line.')

    def data_formatter_commands(self):
        """Test that that file and class static variables display correctly."""
        self.runCmd("file a.out", CURRENT_EXECUTABLE_SET)

        self.expect("breakpoint set -f main.cpp -l %d" % self.line,
                    BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: file ='main.cpp', line = %d, locations = 1" %
                        self.line)

        # This is the function to remove the custom formats in order to have a
        # clean slate for the next test case.
        def cleanup():
            self.runCmd('type format clear', check=False)
            self.runCmd('type summary clear', check=False)

        # Execute the cleanup function during test case tear down.
        self.addTearDownHook(cleanup)

        self.runCmd("type summary add --summary-string \"JustATest\" Point")

        # Simply check we can get at global variables
        self.expect("target variable g_point",
            substrs = ['JustATest'])

        self.expect("target variable g_point_pointer",
            substrs = ['(Point *) g_point_pointer ='])

        # Print some information about the variables
        # (we ignore the actual values)
        self.runCmd("type summary add --summary-string \"(x=${var.x},y=${var.y})\" Point")

        self.expect("target variable g_point",
                    substrs = ['x=',
                               'y='])
        
        self.expect("target variable g_point_pointer",
                    substrs = ['(Point *) g_point_pointer ='])

        # Test Python code on resulting SBValue
        self.runCmd("type summary add --python-script \"return 'x=' + str(valobj.GetChildMemberWithName('x').GetValue());\" Point")

        self.expect("target variable g_point",
                    substrs = ['x='])
        
        self.expect("target variable g_point_pointer",
                    substrs = ['(Point *) g_point_pointer ='])


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
