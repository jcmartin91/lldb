"""Test calling functions in static methods."""

import os, time
import unittest2
import lldb
import lldbutil
from lldbtest import *

class TestObjCStaticMethod(TestBase):

    mydir = os.path.join("lang", "objc", "objc-static-method")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @python_api_test
    #<rdar://problem/9745789> "expression" can't call functions in class methods
    @dsym_test
    def test_with_dsym_and_python_api(self):
        """Test calling functions in static methods."""
        self.buildDsym()
        self.objc_static_method()

    @python_api_test
    #<rdar://problem/9745789> "expression" can't call functions in class methods
    @dwarf_test
    def test_with_dwarf_and_python_api(self):
        """Test calling functions in static methods."""
        self.buildDwarf()
        self.objc_static_method()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line numbers to break inside main().
        self.main_source = "static.m"
        self.break_line = line_number(self.main_source, '// Set breakpoint here.')

    #rdar://problem/9745789 "expression" can't call functions in class methods
    def objc_static_method(self):
        """Test calling functions in static methods."""
        exe = os.path.join(os.getcwd(), "a.out")

        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target, VALID_TARGET)

        bpt = target.BreakpointCreateByLocation(self.main_source, self.break_line)
        self.assertTrue(bpt, VALID_BREAKPOINT)

        # Now launch the process, and do not stop at entry point.
        process = target.LaunchSimple (None, None, os.getcwd())

        self.assertTrue(process, PROCESS_IS_VALID)

        # The stop reason of the thread should be breakpoint.
        thread_list = lldbutil.get_threads_stopped_at_breakpoint (process, bpt)

        # Make sure we stopped at the first breakpoint.
        self.assertTrue (len(thread_list) != 0, "No thread stopped at our breakpoint.")
        self.assertTrue (len(thread_list) == 1, "More than one thread stopped at our breakpoint.")
            
        # Now make sure we can call a function in the static method we've stopped in.
        frame = thread_list[0].GetFrameAtIndex(0)
        self.assertTrue (frame, "Got a valid frame 0 frame.")

        cmd_value = frame.EvaluateExpression ("(char *) sel_getName (_cmd)")
        self.assertTrue (cmd_value.IsValid())
        sel_name = cmd_value.GetSummary()
        self.assertTrue (sel_name == "\"doSomethingWithString:\"", "Got the right value for the selector as string.")

        cmd_value = frame.EvaluateExpression ("[self doSomethingElseWithString:string]")
        self.assertTrue (cmd_value.IsValid())
        string_length = cmd_value.GetValueAsUnsigned()
        self.assertTrue (string_length == 27, "Got the right value from another class method on the same class.")

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
