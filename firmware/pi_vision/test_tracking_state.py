import unittest

from tracking_state import TrackingState


class TrackingStateTests(unittest.TestCase):
    def test_two_hits_confirm_and_short_misses_hold(self):
        state = TrackingState()
        self.assertEqual(state.update((200, 400), 0.0), (500, 500, False))
        self.assertEqual(state.update((250, 450), 0.2), (250, 450, True))
        self.assertEqual(state.update(None, 1.8), (250, 450, True))
        self.assertEqual(state.update((300, 450), 1.9), (300, 450, True))
        self.assertEqual(state.update(None, 3.9), (500, 500, False))

    def test_isolated_hit_does_not_trigger(self):
        state = TrackingState()
        self.assertFalse(state.update((100, 100), 0.0)[2])
        self.assertFalse(state.update(None, 0.8)[2])
        self.assertFalse(state.update((900, 900), 1.0)[2])

    def test_recorded_moving_person_gap_is_held(self):
        state = TrackingState(hold_seconds=3.5)
        state.update((200, 600), 0.0)
        self.assertTrue(state.update((210, 600), 0.2)[2])
        self.assertTrue(state.update(None, 3.53)[2])
        self.assertTrue(state.update((350, 600), 3.54)[2])
        self.assertFalse(state.update(None, 7.04)[2])

    def test_bad_configuration_and_coordinates(self):
        with self.assertRaises(ValueError):
            TrackingState(confirm_hits=0)
        with self.assertRaises(ValueError):
            TrackingState(hold_seconds=0)
        with self.assertRaises(ValueError):
            TrackingState().update((1001, 0), 0.0)


if __name__ == "__main__":
    unittest.main()
